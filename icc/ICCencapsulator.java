/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License"). You may not use
// this file except in compliance with the License. You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description: Code generator for ICC library interfaces 
//              and generator of wrapper functions used by IBM's SSL (GSkit)
// Input:       functions.txt
// Generates:   icc_a.h icc_a.c exports/*  
//              ../iccpkg/* ../iccpkg/exports/* ../iccpkg/muppet.mk
*************************************************************************/

import java.io.*;

// getenv()
import java.lang.System;
import java.util.*;

/**
   This reads a file "functions.txt" full of definitions and generates C source for ICC.
   <p>
   There are currently 7 passes made through the input file.
   <ul>
   <li>pass 1 generates icc_a.c
   <li>pass 2 generates icc_a.h
   <li>pass 3 generates icclib_a.c
   <li>pass 4 generates icclib_a.h AND the export files which limit the 
                    public symbols in ICC
   <li>pass 5 generates iccpkg_a.c 
   <li>pass 6 generates iccpkg_a.h
   # <li>pass 7 generates gsk_wrap.c An interim ICCPKG work-alike
   </ul>
   Note that extra information is generated during some of the passes 
   so we can automagically generate some defines that would otherwise 
   have to be generated later. number_of_functions and number_of_lib_functions
   respectively. That means re-ordering the passes would be bad.
   <p>
   Added. Namespacing: Code to generate macro's in icc_a.h and icclib_a.h which 
   make the API 'look like' the standard ICC API, but actually generate 
   different symbols in the binaries
   <p>
   Added. Literate code: Generate doxygen style comments in generated sources
   from comments in functions.txt
   <p>
   Added. ICCPKG code generation. ICCPKG is actually layered on top of ICC and allows us to
   ship certified and non-certified ICC in one package and use both within a single process.
   However: The code generation is "almost identical" to that of ICC itself, and to avoid
   duplicated maintenance effort/consistancy errors, the automated code generation for ICCPKG
   is also done by this code.
   <p>
   Added. gsk_wrap.c support. GSkit provides the ICC API to ICC consumers to avoid
   having multiple instances of ICC in the same process. 
   As GSkit is binary upgradeable it has to export the non-namespaced ICC API, we do this by
   creating a wrapper with non-namespaced entry points, and use the linker to hide the original
   calls.
   Added. Generate the exports files for GSkit as well as the source and headers.
   <p>
   Use an environment variable to gate which algorithms are selected
   APILEVEL=X (default is 0)
   <p>
   z/OS export files. Note that these need a #pragma(exported_symbol) added to a header,
   so the way that gets handled is different from other platforms.
*/

/*
  Class overview

  ICCencapsulator
  The main class, and holds most static data (strings)
  specific to ICC, plus state information such as the namespace ICC and OpenSSL
  use.
  
  OS
  Contains data and methods for creating export control files 
  specific to each OS variant

  ICCFunction
  Contains data and methods pertaining to function (code) generation

  FileType and subclasses
  Contains data and methods specific to each output type processed.

  There's quite a bit of interaction between the classes - 
  simply because there's a lot of interdependency in reality.
*/

public class ICCencapsulator
{
    // Yes, there are a LOT of 'global' data members here. We could create classes to encapsulate
    // much of this, but it really doesn't gain us anything except performance for code which is
    // not speed critical anyway. (We could reduce the number of times we re-read functions.txt).
    
    //  Current function number. Used as an array index.
    static int funcnum;
    // Number of API entry points
    static int number_of_functions;
    // Number if internal "Meta" library entry points - not the same as the API entry points
    static int number_of_lib_functions;
    // New variable encapsulating the type of operations needed to emit a function for each filetype
    static FileType filetype;
    // Versioning used in the 'whereami' function entry points
    static String ICC_Version;
    // retained List of functions
    static List <ICCFunction> funcs;
    // The current "func" class
    static ICCFunction func; 
    // A list of functionames for this file
    static List <String> functionnames;
    //
    static List <String> osslfunctionnames;
    // Prefix we expect OpenSSL symbols to have
    static String OpenSSLPrefix="";
    // Can be modified via functions.txt PREFIX=<text>
    static String Prefix = "";
    static String ICCPrefix = "ICC_";
    static String METAPrefix = "";
    // Alternate prefixes from a secondary functions.txt
    static String ALT_OpenSSLPrefix="";
    static String ALT_Prefix = "";
    static String ALT_ICCPrefix = "ICC_";
    static String ALT_METAPrefix = "";
    // Types of the ICC control blocks used at different levels 
    static String ICCPCB = "ICC_CTX *pcb";
    static String LIBPCB = "ICClib *pcb";
    // Ugly this, the enum table is generated when we generate icclib_a.c
    // but saved at that point, and we write it into icclib_a.h on the next pass.
    static String enumTable;
    // Deal with the few functions that need to be specified 
    // in namespaces manually. 
    // @Prefix@ will be substituted with the value of Prefix (above)
    // name of the current parsed function
    static String functionname = ""; 

    // Input buffer.
    static int BUFFERSIZE = 4096;

    static boolean debug;
    static String oldcomment = "";
    static boolean oldICC = false;
	// Functions we don't want exported from the step library or implicitly, in icc.h
	static String GSKUnexports[] = { "OS_helpers" };
    /**
     * Parse input read from function.txt, this just keeps slurping data until it has enough
     * @param buf Input buffer
     * @param primary true if this is the first file processed
     * @return the position in the input buffer where we stopped.
     */ 
 	static int parsefile(char[] buf, boolean primary) throws Exception {
		int newstart = 0;
		String temp = new String(buf);
		StringTokenizer tokenizer = new StringTokenizer(temp, ";", true);
		while (tokenizer.hasMoreTokens()) {
			String token = tokenizer.nextToken();
			// System.out.println(token +"\n");

			// okay, now i should have a function
			// or nothing, since the last char should be a ;
			if (tokenizer.hasMoreTokens()) { // then I'm not on my last one, i must have a full function
				// wait, i may have extra whitespace!
				temp = token.trim();
				// System.out.println("token ["+temp+"]");
				if (primary) {
					if (temp.startsWith("PREFIX=")) {
						Prefix = temp.substring(7, temp.length());
						METAPrefix = Prefix + temp.substring(7, temp.length()) + "_";
						ICCPrefix = "ICC" + temp.substring(7, temp.length()) + "_";
					} else if (temp.startsWith("OPENSSLPREFIX=")) {
						OpenSSLPrefix = temp.substring(14, temp.length());
					} else {

						if (0 == func.parse(temp)) { // Found a complete function
							// filetype.Body(func); // emit it's transformed output
							funcs.add(func); // add it to the list of functions
							funcnum++; // increment the function count
							func = new ICCFunction(); // Create a new function
						}
					}
				} else {
					if (temp.startsWith("PREFIX=")) {
						ALT_Prefix = temp.substring(7, temp.length());
						ALT_METAPrefix = ALT_Prefix + temp.substring(7, temp.length()) + "_";
						ALT_ICCPrefix = "ICC" + temp.substring(7, temp.length()) + "_";
					} else if (temp.startsWith("OPENSSLPREFIX=")) {
						ALT_OpenSSLPrefix = temp.substring(14, temp.length());
					} else {
						boolean found = false;
						ICCFunction funcX = new ICCFunction();
						if (0 == funcX.parse(temp)) { // found and parsed a complete function
							for (ICCFunction func : funcs) {
								if (func.name.equals(funcX.name)) {
									found = true;
									if (funcX.numarguments != func.numarguments) {
										System.out.println("Different number of arguments " + funcX.name + " !");
										found = false;
										break;
									} else {
										int k = func.numarguments;
										// Check for argument mismatch - but tolerate pointer / void *
										for (int i = 0; i < k; i++) {
											if (!funcX.argumenttypes[i].equals(func.argumenttypes[i])) {
												System.out.println("Argument type mismatch " + funcX.name + "() '"
														+ func.argumenttypes[i] + "' != '" + funcX.argumenttypes[i] + "'");
												if ( funcX.argumenttypes[i].indexOf("void") >= 0 || func.argumenttypes[i].indexOf("void") >= 0 ) {
													System.out.println("\tok: void");
												}
												else if ( func.argumenttypes[i].indexOf("const") >= 0 ) {
													System.out.println("\tok: const");
												}
												else {
													found = false;
												break;
											}
											}

										}
									}
									// If it's still good, tag that we matched it for the code generator
									if (found) {
										func.taggit();
									}
									break;
								}
							}
							if (!found) {
								System.out.println("Couldn't match legacy " + funcX.name + " !");
							}
						}
					}
				}
				temp = tokenizer.nextToken(); // should be a ;
				// System.out.println("token ["+temp+"]");
				newstart += token.length() + 1;
			} else {
				return newstart;
			}
		}
		// I hit it right on the dot...the last character in the buffer was a ;
		return BUFFERSIZE;
	}

 

    /**
     * @brief Reads one or two functions.txt input files
     * This contains the initialization for the parser
     * and much of the "oh it's that file so I do this" logic.
     * Note that this code originally edited source files - which was bad as
     * any errors tended to clobber existing manual code.
     * We now #include the auto-generated source and headers to avoid that problem.
     * @param inputFile the input file name
     * @param primary is set if this is the defualt functions.txt
     */
   	static void doRead(String inputFile, boolean primary) {

		// set funcnum to the first number the system should use to start
		// automatically distributing enum's for array indices

		funcnum = 0;

		String postamble = new String();

		boolean isMoreLeft = true;
		int retval;
		FileReader myReader;
		File myFile;
		try {
			Long temp;
			// Hack - avoid losing parts of comments by swallowing the folw whole
			// myFile = new File(inputFile);
			// temp = myFile.length();
			// BUFFERSIZE = temp.intValue() -1;
			// Finally fixed the comment bug (The crowd goes wild!)
			myReader = new FileReader(inputFile);
			int appendpoint = 0;
			isMoreLeft = true;

			char[] buf = new char[BUFFERSIZE];
			func = new ICCFunction();
			// Iterate through functions.txt
			while (isMoreLeft) {
				retval = myReader.read(buf, appendpoint, BUFFERSIZE - appendpoint);
				if (retval == -1) {
					// end of file
					System.out.println("Reached end of " + inputFile + " unexpectedly...\n");
					System.exit(1);
				} else if (retval < BUFFERSIZE - appendpoint) { // no more left to read
					isMoreLeft = false;
				}
				int newstart = parsefile(buf, primary);
				// System.out.println(newstart);
				if (newstart == -1) {
					// error
					System.exit(-1);
				} else if (isMoreLeft) {
					for (int i = 0; i < BUFFERSIZE - newstart; i++) {
						buf[i] = buf[newstart + i];
					}
					appendpoint = BUFFERSIZE - newstart;
					for (int i = appendpoint; i < BUFFERSIZE; i++)
						buf[i] = '\0';
				}
			}

			myReader.close();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	static void doWork(FileType filetype) {
		System.out.println(filetype.name);
		try {
			functionnames = new ArrayList<String>();

			filetype.Preamble();
			for (ICCFunction func : funcs) {
				if (func.isMemberOf(filetype)) {
					func.fixTypes(filetype);
					functionnames.add(func.name);
				}
			}
			funcnum = 0;
			for (ICCFunction func : funcs) {
				if (func.isMemberOf(filetype)) {
					filetype.Body(func); // emit it's transformed output
					funcnum++;
				}
			}
			// now to add any one off code at the end.
			filetype.Postamble();
			// close the output file or whatever
			filetype.Cleanup();
		} catch (Exception e) {
			e.printStackTrace();
		}

	}

    /**
     * Simple main entry point. All this does is call doWork() repeatedly to read
     * functions.txt and drive the code generation depending on the
     * value of filenum. 
     * Note that the individual passes won't work stand alone any more, you have to do all the passes,
     * and in the correct order.
     * @param args - array of command line arguments, unused.
     */
    public static void main(String[] args)
    {
	String tmp; 
	PrependWords.SetupPrependWords();
	//Flags determine what type of output will be created
	//Current settings:
	//1 icc_a.c     hasICCprefix,requirespcb,callsMETAprefix
	//2 icc_a.h     isheader,hasICCprefix,requirespcb,callsMETAprefix
	//3 icclib_a.c  hasMETAPrefix
	//4 icclib_a.h  isheader,hasMETAPrefix
	//5 iccpkg_a.c 
	//6 iccpkg_a.h
	//7 gsk_wrap2.c
	//8 jgsk_wrap2.c
	//9 jcc_a.h

	// D40180 ECC is now "always on" so no need for this check.
	// tmp = System.getenv("APILEVEL");
	// if( tmp != null ) {
	//     APILEVEL = Character.digit(tmp.charAt(0),10);
	// }

		try {
			ICC_Version = ICCVersion.GetVersion();
			funcs = new ArrayList<ICCFunction>();
			// System.out.println("ICC version ["+ICC_Version+"]");
			doRead("functions.txt", true);

			if (args.length > 0) {
				oldICC = true;
				doRead(args[0], false);
			}

			doWork(new File_ICC_A_C());

			// Save this here, we need to emit a define for this value
			// at the START of the header
			number_of_functions = functionnames.size();

			doWork(new File_ICC_A_H()); // icc_a.h

			doWork(new File_ICCLIB_A_C()); // icclib_a.c
			// Save the number of library functions
			number_of_lib_functions = functionnames.size();

			doWork(new File_ICCLIB_A_H()); // icclib_a.h

			doWork(new File_ICCPKG_A_C()); // iccpkg_a.c

			doWork(new File_ICCPKG_A_H()); // iccpkg_a.h
			/*
			 * doWork(new File_GSKWRAP_C()); // gsk_wrap.c
			 */
			doWork(new File_GSKWRAP2_C()); // gsk_wrap2_a.c

			doWork(new File_Muppet_mk()); // muppet.mk

			try {
				doWork(new File_one_sh()); // one.sh
			}
			catch (FileNotFoundException e) {
				System.out.println("Failed: out.sh");
			  // ignore
			}

			// Extra functions to support replacement of
			// OpenSSL in various language backends
			doWork(new File_ICC_AUX_A_C()); // icc_aux_a.c

			doWork(new File_ICC_AUX_A_H()); // icc_aux_a.h

			doWork(new File_JCC_A_H()); // Header for jgsk_wrap2_a.c
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
class PrependWords {
    private static List<String> prependwords;

    /**
     * Create the list of OpenSSL keywords that need massaging 
     * to prepend ICC_ to them. These are generally tags for opaque
     * data types.
     * It's done this way as Java has no convenient constructor that'll
     * do this with less effort and the initialization needs to be
     * "near the front" for ease of maintenance.
     */
 	public static void SetupPrependWords() {
		// Order these so that the longest are first
		// as the matching order is first->last
		// That avoids doing replacements when we hit substrings
		// which may match the keywords

		prependwords = new ArrayList<String>();
		prependwords.add("PKCS8_PRIV_KEY_INFO");
		prependwords.add("EC_builtin_curve");
		prependwords.add("ECDSA_METHOD");
		prependwords.add("ECDH_METHOD");
		prependwords.add("ASN1_OBJECT");
		prependwords.add("X509_ALGOR");
		prependwords.add("ECDSA_SIG");
		prependwords.add("EC_METHOD");
		prependwords.add("EC_POINT");
		prependwords.add("EC_GROUP");
		prependwords.add("PRNG_CTX");
		prependwords.add("AES_GCM");
		prependwords.add("DSA_SIG");
		prependwords.add("EC_KEY");
		prependwords.add("BIGNUM");
		prependwords.add("PRNG");
		prependwords.add("CMAC");
		prependwords.add("EVP_KDF");
		prependwords.add("OSSL_PARAM");
		prependwords.add("HMAC");
		prependwords.add("KDF");
		prependwords.add("DES");
		prependwords.add("DSA");
		prependwords.add("EVP");
		prependwords.add("RSA");
		prependwords.add("BN");
		prependwords.add("DH");
		// System.out.println(prependwords);

	}

	static public List<String> words() {
		return prependwords;
	}
}
// filenum "enum"'s to make the pass selection logic saner.
// (Could probably be made to go away with cleaner design)
enum Filenum {
    ICC_A_C,
    ICC_A_H,
    ICCLIB_A_C,
    ICCLIB_A_H,
    ICCPKG_A_C,
    ICCPKG_A_H,
    GSK_WRAP_C,
    GSK_WRAP2_C,
    ICC_AUX_A_C,
	ICC_AUX_A_H,
	JCC_A_H,
}

// Class encapsulating the operations/transforms expected
// to be carried out for different output file types
//
// Note, functions in here are used
// by multiple FileType subclasses rather than being
// in the subclass. 
// To be fair - this does reduce maintenance of multiple 
// text blocks
//
class FileType {
    public Filenum fn;
    // Most of the vars below should evaporate once refactored.
    public boolean isheader = false;
    public boolean isdll = false;
    public boolean debug = false;
    public boolean hasMETAprefix = false;
    public boolean hasICCprefix = false;
    public boolean requiresiccpcb = false;
    public boolean requiresicclibpcb = true;
    public boolean passesiccpcb = false;
    public boolean passesicclibpcb = false;
    public boolean requiresdeclspec = false;
    public boolean callsICCprefix = false;
    public boolean callsMETAprefix = false;
    public String pcbtype = "";
    public String prefix = "";
    FileWriter writer;
    public String name = "";
    static final String copyrightheader = 
        "/*-----------------------------------------------------------------\n"+
		"// Copyright IBM Corp. 2023\n"+
		"//\n"+
		"// Licensed under the Apache License 2.0 (the \"License\"). You may not use\n"+
		"// this file except in compliance with the License. You can obtain a copy\n"+
		"// in the file LICENSE in the source distribution.\n"+
        "//----------------------------------------------------------------*/\n\n\n";
    static final String preambleend = "/* Machine generated code: DO NOT EDIT */";
    static final String postamble = "/* Machine generated code: DO NOT EDIT */";

    static final String MiscFunctions [] = {
        "#define ICC_lib_cleanup ICC@Prefix@_lib_cleanup",
        "#define META_CRYPTO_mem_ctrl META@Prefix@_CRYPTO_mem_ctrl"
    };

    // Functions which aren't in functions.txt but which must
    // be namespaced ICC_init/ICC is the only one now.
    static String ExtraFunctions[] = {
        "#define ICC_Init ICC@Prefix@_Init\n",
	"/*! @brief Obtain an ICC context\n"+
	" *  @param status a pointer to previously allocated ICC_STATUS structure\n"+
	" *  @param iccpath a string containing the root path to the ICC shared libraries\n"+
	" *  @note  ICC internally adds icc/icclib/[icc libname] icc/osslib/[openssl libname]\n"+
	" *  to the iccpath provided to locate the actual libraries\n"+
	" *  @return An ICC_CTX pointer or NULL on failure\n"+
	" */\n",
        "ICC_CTX * ICC_LINKAGE ICC@Prefix@_Init(ICC_STATUS* status,const char* iccpath);\n\n",
        "#if defined(_WIN32)\n"+
        "/* Should only be needed on Windows ... Unicode version of ICC_Init */\n",
        "#define ICC_InitW ICC@Prefix@_InitW\n",
	"/*! @brief Obtain an ICC context\n"+
	" *  @param status a pointer to previously allocated ICC_STATUS structure\n"+
	" *  @param iccpath a UNICODE string containing the root path to the ICC shared libraries\n"+
	" *  @note  ICC internally adds icc/icclib/[icc libname] icc/osslib/[openssl libname]"+
	" to the iccpath provided to locate the actual libraries\n"+
	" *  @return An ICC_CTX pointer or NULL on failure\n"+
	" */\n",
        "ICC_CTX * ICC_LINKAGE ICC@Prefix@_InitW(ICC_STATUS* status,const wchar_t* iccpath);\n",
        "#endif\n",

    };

    static final String GSKExtraFunctions[] = {
	"\n"+
	"/*! @brief Find the full path to ICC needed to give to the ICC_Init call\n"+
	" *  @param return_path input buffer to contain the returned path\n"+
	" *  @param path_len max length to copying into return_path\n"+
	" *  @return The path length on sucess,0 on failure, -1 on a parameter error\n"+
	" */\n"+
	"int ICC_LINKAGE gskiccs_path(char *return_path, int path_len);\n\n"+

	"\n",
	"#if defined(_WIN32)\n"+
	"/*! @brief Find the full path to ICC needed to give to the ICC_InitW call\n"+
	" *  @param return_path input buffer to contain the returned path\n"+
	" *  @param path_len max length to copying into return_path\n"+
	" *  @return The path length on sucess,0 on failure, -1 on a parameter error\n"+
	" */\n"+
	"int ICC_LINKAGE gskiccs_pathW(wchar_t *return_path, int path_len);\n\n"+
	"#endif\n"+
	"\n"
    };

    FileType(Filenum filenum)
    {
		SetType(filenum);
    }
    
	public void SetType(Filenum filenum) {
		fn = filenum;
		isheader = false;
		isdll = true;
		debug = false;
		switch (filenum) {
		case ICC_AUX_A_C:
		case ICC_A_C:
		case ICCLIB_A_C:
		case ICCPKG_A_C:
		case GSK_WRAP_C:
		case GSK_WRAP2_C:
			isheader = false;
			break;
		default:
			isheader = true;
			break;
		}
		switch (filenum) {
		case ICC_AUX_A_H:
		case ICCLIB_A_C:
		case ICCLIB_A_H:
		case JCC_A_H:
			isdll = true;
			break;
		default:
			isdll = false;
			break;
		}

		if (isdll) {
			hasICCprefix = false;
			hasMETAprefix = true;
			requiresiccpcb = false;
			requiresicclibpcb = true;
			passesiccpcb = false;
			passesicclibpcb = false;
			requiresdeclspec = false;
			callsICCprefix = false;
			callsMETAprefix = false;

		} else {
			hasICCprefix = true;
			hasMETAprefix = false;
			requiresiccpcb = true;
			requiresicclibpcb = false;
			passesiccpcb = false;
			passesicclibpcb = true;
			requiresdeclspec = false;
			callsICCprefix = false;
			callsMETAprefix = true;
		}
	}

    public void Preamble() throws Exception
    {	
	writer.write("\n"+copyrightheader+preambleend+"\n\n");
    }

	public void Body(ICCFunction func) throws Exception {
		// generate any comments etc
		emitHeader(func);
		// Generate the "fixed stuff" common to each function
		emitFunctionPreamble(func);
		// There's special case processing needed for some functions, that's handled in
		// here
		emitFunctionBody(func);
	}
    public void Postamble() throws Exception
    {
	writer.write("\n"+postamble+"\n");
    } 
    public void Cleanup() throws Exception
    {
	writer.close();
    }
    //////////////////////////////////////////////////////////////////////////////////
    // Per function operations
    //////////////////////////////////////////////////////////////////////////////////
    /**
     * emit the typedef/comments/function head for a relayed function call
     * @param writer output stream
     */
    public void emitFunctionPreamble(ICCFunction func) throws Exception
    {   
		func.WriteTypedef(writer,passesiccpcb || (passesicclibpcb && func.usespcb));

		// Is documentation available ?
		writer.write(func.GenComment(this));
 
    }
    /**
     * @brief emit the body of a relayed function call.
     * Note that the functions this calls may often be reimplemented
     * if derived classes - much of this is reusable however
     * @param myWriter output stream
     */
	public void emitFunctionBody(ICCFunction func) throws Exception {
		String pcb = "";

		func.WriteFunction(writer, prefix, pcbtype);
		// Start of function body
		writer.write("\n{\n");
		// Write tempory value for a return value if the called function
		// has one
		func.WriteRV_TEMP(writer);
		// Does this function need a PCB passed ?, if so check here that
		// wasn't NULL
		if (requiresiccpcb || (requiresicclibpcb && func.usespcb)) {
			writer.write("\t/* Note PCB is always checked for NULL in the calling function */\n");
			writer.write("\tif (NULL != pcb->funcs) {\n");
		} // Create a temp for the indirect call so we can test and jump safely in the
			// case that the function tables
		// are swapped due to an error in another thread
		writer.write("\t\t" + func.typedefname + " tempf = (" + func.typedefname + ")(*(pcb->funcs))["
				+ ICCencapsulator.funcnum + "].func;\n");

		// Write code for conditional tests executed before we call the function
		func.WriteBodyPreamble(writer, requiresicclibpcb && func.usespcb, func.genGlobal(this), genEnum(func.name));

		if ((passesiccpcb && requiresiccpcb) || (passesicclibpcb && requiresicclibpcb)) {
			writer.write("(void*)pcb");
		}
		// Oops, an error "shouldn't happen" combination ....
		if (passesiccpcb && requiresicclibpcb) {
			System.out.println("Cannot get ICC pcb from ICClib pcb!\n");
		}
		// set up to call the function indirectly via the pcb passed in
		if (passesicclibpcb && requiresiccpcb && func.usespcb)
			pcb = "(void*)pcb->funcs";
		func.WriteCallingFunctionBody(writer, pcb);
		// Terminate function call statement
		writer.write(";\n");
		// a dummy in the base class, but overridden is derived classes.
		writeAnyExtraCodeInFunction(func);
		writer.write("\t\t}\n");
		// Check that we don't try to return an error status
		// for functions with void return types
		if (requiresicclibpcb) {
			func.WriteRVWarning(writer);
		}
		//
		writer.write("\t}\n");
		func.WriteReturn(writer);
		// End of function
		writer.write("}\n\n");
		writeAnyExtraFunction(func);
	}

    /**
     * @brief Do any extra processing after the indirect function call
     * This is overridden in derived classes needing to do this
     * @param func the function being processed 
     */
    public void writeAnyExtraCodeInFunction(ICCFunction func) throws Exception
    {

    }
    /**
     * @brief Do any extra processing after the indirect function 
     * was instantiated i.e. a related function call
     * This is overriden in derived classes needing to do this
     * @param func the function being processed 
     */
    public void writeAnyExtraFunction(ICCFunction func) throws Exception
    {

    }

    /**
     * @brief Write a header file from the previously parsed line of data
     * @param func The function class
     */
    public void emitHeader(ICCFunction func) throws Exception
    {
	String pref = "";
	if(hasMETAprefix) pref = ICCencapsulator.METAPrefix;
	else if(hasICCprefix) pref = ICCencapsulator.ICCPrefix;
	func.putLegacy(writer);
	// if there's a doxygen style comment for this function emit it
	writer.write(func.GenComment(this));

	func.WriteFunction(writer,pref,pcbtype);
	writer.write(";\n\n");

    }

    ///////////////////////////////////////////////////////////////////////////////
    // File wide operations
    /**
     * Generates a string which is name of the global structure holding
     * shared library information/call vectors 
     * for either the ICC or OpenSSL libraries
     * @param func The root function name
     * @return The input converted to an enum
     */
    String genGlobal()
    {
        String rv = new String("");
        if (hasICCprefix == true) rv = "ICCGlobal";
        else if (hasMETAprefix == true) rv= "Global";
        else rv = "__BROKEN__";
        return rv;

    }
    /**
     *Generates the name of the library handle to use
     * @return the generated library handle name
     */
    String genLibHandleName()
    {
        String rv = new String("");
	switch(fn) {
	case ICC_A_C:
	case ICCPKG_A_C:
	    rv = "ICCGlobal.h";
	    break;
	default:
	    rv = "Global.hOSSLib";
	    break;
	}
        return rv;
       
    }    // Somewhat common code between icc_a.c and icc_lib_a.c
    void writeInitFunction(String myPrefix) throws Exception
    {

    }
    /**
     * Handle the couple of functions which fall through the gaps and
     * if this is a known non-FIPS release of the code, 
     * emit a symbol which tells us this
     * @param myWriter output stream
     */
    void writeMischeaderStuff()  throws Exception
    {
        int i;    
        String c;

        // Check to see if we are doing a namespaced version
		if ( ! ICCencapsulator.ICCPrefix.equals("ICC_") ) {
	
	    	writer.write("/* Non-public API functions, do not access via user code */\n");
	    	/* CMVC 40048 made these private but we still need the definitions */
	    	for (String extra :MiscFunctions) {
				c = HandlePrefixInDefine(ICCencapsulator.Prefix,extra);
				writer.write(c+"\n");
	    	}	
	    
	    	// This only gets written if ICCPrefix is set, i.e. it's a non-FIPS release.
	    	// We use this define to disable the ability to get into FIPS mode
	    	if (  ICCencapsulator.Prefix.indexOf("C") < 0 ) {
				writer.write("#define lib_init N_lib_init\n");
				writer.write("#define NON_FIPS_ICC 1\n");
	    	} else {
				writer.write("#define lib_init C_lib_init\n");
				writer.write("#define NON_FIPS_ICC 0\n");
			}	
	    	writer.write("/* End Non-public API function */\n");
		}
    }

    /**
     * Parses Strings to handle namespacing
     * i.e. "#define Xyz @Prefix@Xyz" 
     * Expands the prefix, generates the #define, but also
     * generates a reference to the real implementation
     * @param s string to parse
     * @return the parsed and reconstructed string
     */
    static String HandlePrefixInDefine(String prefix,String s) 
    {
        int i;
        int j;
        String a;
        String b;
        String c;
        j = s.indexOf("@Prefix@");
        if(j >= 0) {
            a = s.substring(0,j);
            b = s.substring(j + 8);
            if ( s.indexOf("#define") >= 0) {
                // FIXME
                for ( i = j; i > 0 &&  s.charAt(i) != ' '; i--);            
                c = "/*! \\sa "+ s.substring(i,j) + prefix + b + " */\n";
            } else {
                c = "";
            }
            c = c + a + prefix + b;
            // System.out.println("a = <"+a+"> b = <"+b+"> c = <"+c+">\n");
        } else {
            c = s;
        }
        return c;
    }
   /**
     * Handle any extra functions needed by GSkit
     * and as a result don't have good autogenerated info.
     * @param myWriter output stream
     * @param defines if true, process #defines, if false, leave them out
     */
    void writeGSKitExtraHeaderStuff()  throws Exception
    {
	for (String func: GSKExtraFunctions) {
	    writer.write(func);	    
	}	
    }
    /**
     * Handle the couple of functions which fall through the gaps
     * and as a result don't have good autogenerated info.
     * @param myWriter output stream
     * @param defines if true, process #defines, if false, leave them out
     */
    void writeExtraHeaderStuffUnCon(String prefix,boolean defines)  throws Exception
    {
	int i;
	String c;
	if(defines) writer.write("/* Namespacing of Public API functions */\n");
	for (String func: ExtraFunctions) {
	    if( defines || func.indexOf("#define") < 0 ) {
		// Insert a reference to the documentation for the namespaced function.
		c = HandlePrefixInDefine(prefix,func);
		writer.write(c+"\n");
	    }
	}	
	if(defines) writer.write("/* End namespacing of public API functions */\n");
    }


    /**
     * Handle the couple of functions which fall through the gaps
     * and as a result don't have good autogenerated info.
     * @param myWriter output stream
     * @param defines if true, process #defines, if false, leave them out
     */
    void writeExtraHeaderStuff(boolean defines)  throws Exception
    {
        // Check to see if we are doing a namespaced version
        if ( ICCencapsulator.Prefix != "" ) {
	    	writeExtraHeaderStuffUnCon(ICCencapsulator.Prefix,defines); 
        }
    }




    /**
     * Writes defines used by the FIPS startup code which needs to call OpenSSL 
     * before we are fully alive and kicking.
     * Again, it's a little ugly moving this to here, but less ugly than having yet another
     * autogenerated file.
     * @param myWriter output stream
     */
    void writeFIPSheaderStuff()  throws Exception
    {	

        writer.write(
                       "\n\n#if defined(INCLUDED_FIPS)\n"+
                       "\n/*The following defines are needed by the fips mode startup code*/\n"+
                       "#define META_EVP_SignInit(a,b,c) "+
                       ICCencapsulator.METAPrefix+"EVP_DigestInit(a,b,c)\n"+
                       "#define META_EVP_SignUpdate(a,b,c,d) "+
                       ICCencapsulator.METAPrefix+"EVP_DigestUpdate(a,b,c,d)\n"+
                       "#define META_EVP_VerifyInit(a,b,c) "+
                       ICCencapsulator.METAPrefix+"EVP_DigestInit(a,b,c)\n"+					   
                       "#define META_EVP_VerifyUpdate(a,b,c,d) "+
                       ICCencapsulator.METAPrefix+"EVP_DigestUpdate(a,b,c,d)\n"+	
                       "\n"+                     
                       "#endif\n"
                       );
    }

     /** 
     * Generate the very large string for the enum tables used in ICC
     * We generate this as a string, as it must be generated by the pass
     * that creates icclib_a.c, but written into icclib_a.h
     * For icc_a.c/icc_a.h that problem doesn't arise, the enum table 
     * is private information and belongs in the C source.
     */
	String generateEnum() throws Exception {
		String tmp;
		tmp = "/*! @brief enum's for function table indices */\n"
				+ "/* Note: These get generated AFTER they could be used\n"
				+ "   in some of the code above (pcb-funcs[32].func) etc.\n"
				+ "   The problem is that'd require an extra pass as we don't\n"
				+ "   know all the values for this table until all functions\n"
				+ "   have been parsed. Since this is autogenerated (reliable) I\n"
				+ "   decided this was acceptable.\n" + "*/\n";
		tmp += "typedef enum\n{\n";
		int j = ICCencapsulator.functionnames.size();
		for (int i = 0; i < j; i++) {
			tmp += "\t" + genEnum(ICCencapsulator.functionnames.get(i)) + " = " + i + ",\n";
		}
		tmp += "\t" + genEnum("TableEnd") + "\n";
		if (hasICCprefix)
			tmp += "} ICC_FUNCTION_ENUM;\n";
		else if (hasMETAprefix)
			tmp += "} META_FUNCTION_ENUM;\n";
		else
			tmp += "} SHOULDNT_HAPPEN_ENUM;\n\n";
		return (tmp);
	}
     /**
     * Generates a string which is the enum text for the various function pointer entries.
     * @param func The root function name
     * @return The input converted to an enum
     */
    String genEnum(String func)
    {
        String rv = new String("");

        rv = "indexOf_"+func;
        return rv;

    }


} // End base FileType cass

// FileType to write icc_a.c

class File_ICC_A_C extends FileType
{

    File_ICC_A_C() throws Exception
    {
	super(Filenum.ICC_A_C);
	pcbtype = ICCencapsulator.ICCPCB;
	name = "icc_a.c";
	writer = new FileWriter(name);
    }
    public void Preamble() throws Exception
    {  
	writer.write("\n#if defined(ICC)\n");
	super.Preamble();	
    }
    public void Body(ICCFunction func) throws Exception
    {
	prefix = ICCencapsulator.ICCPrefix;
	// Suppress ICC_SetValue manually.
	if(func.name.equals("SetValue") ) {
	    func.WriteTypedef(writer,passesiccpcb || passesicclibpcb);
	} else {
	    super.Body(func);
	}
    }
    public void Postamble() throws Exception
    {
	writer.write("\n#endif /* defined(ICC) */\n");
	// writer.write("\n#if defined(ICCLIB)\n");
	// writeStaticICCGlobalStructure(ICCencapsulator.METAPrefix);
	// writer.write("\n#endif /* defined(ICCLIB) */\n");
	ICCencapsulator.enumTable = generateEnum();
 	writer.write(ICCencapsulator.enumTable);

	
	writeInitFunction(ICCencapsulator.METAPrefix);
	
	super.Postamble();

    }
    // Suppress function prototypes in the C file
    public void emitHeader(ICCFunction func)
    {
    }


  /**
     * Generates the static header definition for the global/per-process ICCLib data
     * This is esentially the hooks into OpenSSL, which get populated once by the first
     * sucessfull load of the ICC libraries. 
     * Note that this may be de-populated as well !. 
     * For example: if the library signature check fails this structure will be invalidated
     * and we'll attempt to re-populate it again next time.
     */
    private void writeStaticICCGlobalStructure(String myPrefix) throws Exception
    {

	writer.write("\n/*! @brief this is the default data structure"+
		     "\n    that holds the call table for ICC"+
		     "\n*/"+
		     "\nstatic FUNC ICCGlobal_default[NUM_ICCFUNCTIONS] ="+
		     "\n{");
	for (String func: ICCencapsulator.functionnames) {
	    writer.write("\n\t{\"" + func +"\",NULL},");
	}
	writer.write("\n};\n");


		      
    }

}

// FileType to write icc_a.h
class File_ICC_A_H extends FileType {

	File_ICC_A_H() throws Exception {
		super(Filenum.ICC_A_H);
		pcbtype = ICCencapsulator.ICCPCB;
		name = "icc_a.h";
		writer = new FileWriter(name);

	}

	public void Preamble() throws Exception {
		super.Preamble();
		writer.write("/** \\file icc_a.h\n" + "* Function prototypes for the ICC API (ICCSDK).\n"
				+ "* This file is autogenerated and should only be included via icc.h.\n" + "*/\n\n");

		writer.write("\n#ifndef INCLUDED_ICC_A\n#define INCLUDED_ICC_A\n");

		writer.write("\n#define NUM_ICCFUNCTIONS " + ICCencapsulator.number_of_functions + "\n\n");

		writer.write("#if !defined(ICCLIB)\n");
	}

	public void Body(ICCFunction func) throws Exception {
		prefix = ICCencapsulator.ICCPrefix;

		if(! func.javaonly) {
			emitHeader(func);
		}	
	}

	public void Postamble() throws Exception {

		writeExtraHeaderStuff(true);
		writeMischeaderStuff();

		writer.write("\n#endif /*INCLUDED_ICC_A*/\n");

		super.Postamble();
		writer.write("#endif /*!defined(ICCLIB) */\n");
	}

	public void emitHeader(ICCFunction func) throws Exception {
		emitNamespacedDefines(func);
		super.emitHeader(func);
	}

	public void emitNamespacedDefines(ICCFunction func) throws Exception {
		if (ICCencapsulator.ICCPrefix != "") {
			writer.write("/*! \\sa " + ICCencapsulator.ICCPrefix + func.name + "*/\n");
			writer.write("#define ICC_" + func.name + " " + ICCencapsulator.ICCPrefix + func.name + "\n");
		}
	}
}


// FileType to write icclib_a.c
class File_ICCLIB_A_C extends FileType
{

    File_ICCLIB_A_C() throws Exception
    {
	super(Filenum.ICCLIB_A_C);
	pcbtype = ICCencapsulator.LIBPCB;
	name = "icclib_a.c";
	writer = new FileWriter(name);

    }
    public void Preamble() throws Exception
    {
	super.Preamble();	
    }
    public void Body(ICCFunction func) throws Exception
    {
	/*
	prefix = ICCencapsulator.METAPrefix;
	super.Body(func);
	*/
    }
    
    // Suppress function prototypes in the C file
    public void emitHeader(ICCFunction func)
    {
    }
	
    /**
     * @brief Do any extra processing after the indirect function call
     * In this case FIPS mandated consistancy tests on created keys
     * and various fixups.
     * @param func the function being processed 
     */
    public void writeAnyExtraCodeInFunction(ICCFunction func) throws Exception
    {
	
    }
    /**
     * @brief Do any extra processing after the main indirect function has been written
     *  i.e. a related function call
     * in this case if we have an "ef" function, create it
     * @param func the function being processed 
     */
    public void writeAnyExtraFunction(ICCFunction func) throws Exception
    {
 	// Direct access functions - same pattern as above, but only in icclib_a.c zero checking and no ICClib etc
	if(func.macrofunction == true) {
	    writer.write("/*\n * This version of the previous function is used internally,\n"+
			   " * either during startup\n" +
			 " * or by an OpenSSL callback function when ICC contexts are\n" +
			   " * unavailable.\n"+
			 " */\n"
			 );
	    func.WriteMacroFunction(writer);
	    writer.write("\n{\n");
	    func.WriteRV_TEMP(writer);	
	    func.WriteReturnEQ(writer);
	    func.WriteIndirectCall(writer,func.genGlobal(this));
	    func.WriteCallingFunctionBody(writer,"");
	    writer.write(";\n");
	    func.WriteReturn(writer);
	    writer.write("}\n\n");
	}
    }
    
    public void Postamble() throws Exception
    {
	
	// Generate the static global structure which holds the OpenSSL call tables
	// This needs work as it now needs to export some extra functions
	writeStaticGlobalStructure(ICCencapsulator.OpenSSLPrefix);
	ICCencapsulator.enumTable = generateEnum();
	// Write the code that populates the global call table
	writeInitFunction(ICCencapsulator.OpenSSLPrefix);	
	writeGlobalDefaultStructure("");
	super.Postamble();
	OS os = new OS();
	// os.write_OSSLexports(ICCencapsulator.OpenSSLPrefix,ICCencapsulator.functionnames);

    }
  /**
     * Generates the static header definition for the global/per-process ICCLib data
     * This is esentially the hooks into OpenSSL, which get populated once by the first
     * sucessfull load of the ICC libraries. 
     * Note that this may be de-populated as well !. 
     * For example: if the library signature check fails this structure will be invalidated
     * and we'll attempt to re-populate it again next time.
     */
	private void writeGlobalDefaultStructure(String myPrefix) throws Exception {

		writer.write("\n/*! @brief this is the default data structure" + "\n    that holds the call table for icclib"
				+ "\n*/" + "\nstatic FUNC ICCGlobal_default[NUM_ICCLIBFUNCTIONS] =" + "\n{");
		for (String func : ICCencapsulator.functionnames) {
			writer.write("\n\t{\"" + func + "\",NULL},");
		}
		writer.write("\n};\n");

	}

     /**
     * Generates the static header definition for the global/per-process ICCLib data
     * This is esentially the hooks into OpenSSL, which get populated once by the first
     * sucessfull load of the ICC libraries. 
     * Note that this may be de-populated as well !. 
     * For example: if the library signature check fails this structure will be invalidated
     * and we'll attempt to re-populate it again next time.
	 * @param myPrefix The default prefix to stick on function names
     */
	void writeStaticGlobalStructure(String myPrefix) throws Exception {

		writer.write("\n/*! @brief This is the global structure that holds the "
				+ "\n           crypto library specific data."
				+ "\n           Once it's loaded and the library has been validated"
				+ "\n           the first time we don't need to touch this again." + "\n*/"
				+ "\nstruct ICClibGlobal_t Global = {" + "\n\t\"ICC\", /*!< ID, Always ICC */"
				+ "\n\t\"\",    /*!< version */" + "\n\t\"\",    /*!< load path */"
				+ "\n\tNULL,    /*!< OpenSSL library handle, now unused */" + "\n\t{");
		for (ICCFunction func : ICCencapsulator.funcs) {
			if (ICCencapsulator.functionnames.contains(func.name)) { /* Horribly inefficient */
				/* But the payoff is here, handling functions we have to redirect */
				if (func.redirect) {
					writer.write("\n\t\t{\"" + func.name + "\",(PFI)my_" + func.name + "},");
				} else {
					writer.write("\n\t\t{\"" + func.name + "\",(PFI)" + myPrefix + func.name + "},");
				}
			}
		}
		/*
		 * for (String func : ICCencapsulator.functionnames) { writer.write("\n\t\t{\""
		 * + func +"\",(PFI)"+ myPrefix + func + "},"); }
		 */
		writer.write("\n\t\t{NULL,NULL},"); /* End of function list */
		writer.write("\n\t}\n,");
		writer.write("\n\t0,      /*!< unicode flag */" + "\n\t0       /*!< Initialized , i.e. POST run etc */"
				+ "\n};\n\n");
	}
 

}
// FileType to write icclib_a.h
class File_ICCLIB_A_H extends FileType
{

    File_ICCLIB_A_H() throws Exception
    {
	super(Filenum.ICCLIB_A_H);
	pcbtype = ICCencapsulator.LIBPCB;
	name = "icclib_a.h";
	writer = new FileWriter(name);
	
    }
    public void Preamble() throws Exception
    {
	super.Preamble();
	writer.write("\n/* Avoid symbol clashes between namespaced ICC's */\n"+
		     "\n#define ICC_SCCSInfo ICC"+ICCencapsulator.Prefix+"_SCCSInfo\n\n"
		     );
	write_ICClib_t();
    }
    public void Body(ICCFunction func) throws Exception
    {

    }
    public void Postamble() throws Exception
    {
	// Last pure ICC file, write the exports files
	OS os = new OS();
	// Note that ICCencapsulator.functionnames is a dummy now
	// we only export lib_init

	os.write_ICCexports(ICCencapsulator.METAPrefix,ICCencapsulator.functionnames);	
	// We use this define to disable the ability to get into FIPS mode
	if (  ICCencapsulator.Prefix.indexOf("C") < 0 ) {
		writer.write("#define lib_init N_lib_init\n");
	    writer.write("#define NON_FIPS_ICC 1\n");
	}  else {
		writer.write("#define lib_init C_lib_init\n");
		writer.write("#define NON_FIPS_ICC 0\n");
	}

	
    }
    public void emitHeader(ICCFunction func) throws Exception
    {


    }
    public void emitNamespacedDefines(ICCFunction func)  throws Exception
    {

    }
     /**
     * Write the definition of ICClib_t to an output stream (always icclib_a.h)
     * This is a separate function simply so I could de-cruft doWork()
     * @param myWriter The stream to write to.
     */
	void write_ICClib_t() throws Exception {

		writer.write("\n#define NUM_ICCLIBFUNCTIONS " + (ICCencapsulator.number_of_lib_functions + 1) + "\n\n");
		writer.write("\n/*! @brief The definition of the global static library hook part of ICClib\n"
				+ "             Only one instance exists which is populated only once, by the first\n"
				+ "             ICC_Attach() call which loads and validates ICC and OpenSSL libraries\n" + "\n*/\n");
		writer.write("\nstruct ICClibGlobal_t\n{\n\t" + "char ID[4];                          /*!< set to ICC */\n\t"
				+ "char version[20];                    /*!< set to the ICC version i.e. 1.2 */\n\t"
				+ "char iccpath[MAX_PATH*4];            /*!< set to the ICC path we loaded ICC from and large enough to hold uc32 strings */\n\t"
				+ "void *hOSSLib;                       /*!< handle of OpenSSL library (from dlopen()) */\n\t"
				+ "FUNC funcs[NUM_ICCLIBFUNCTIONS];        /*!< An array of them, one for each function */\n\t"
				+ "int unicode;                         /*!< Unicode init path (iccpath) */\n\t"
				+ "int initialized;                     /*!< Initialized, POST, integrity checks completed */\n\t"
				+ "ICC_STATUS status;                   /*!< Global status. Needed since POST etc happen during library load now and we need to preserve errors */\n\t"
				+ "ICC_Mutex mtx;                       /*!< Global mutex, mainly needed to keep thread safety debug tools from complaining*/\n\t"
				+ "};\n" + "\n\n");

		writer.write("/*! @brief ICClib_t is used to hold the per-instance ICC context info.\n"
				+ "           This information is opaque to ICC users\n" + "*/\n");
		writer.write("\nstruct ICClib_t\n{\n" + "\tFUNC *funcs;\n"
				+ "\tint length;                          /*!< sizeof ICClib_t (myself) */\n"
				+ "\tchar pIDinit[8];                     /*!< Process ID at ICC_Init */\n"
				+ "\tchar tIDinit[8];                     /*!< Thread ID at ICC_Init */\n"
				+ "\tchar toi[8];                         /*!< creation time i.e. time() */\n"
				+ "\tchar pIDattach[8];                   /*!< Process ID at ICC_Attach */\n"
				+ "\tchar tIDattach[8];                   /*!< Thread ID at ICC_Attach */\n"
				+ "\tchar toa[8];                         /*!< attach time. i.e. time() */\n"
				+ "\tint flags;                           /*!< mode flags. FIPS, ERROR etc*/\n"
				+ "\tint lock;                            /*!< Set once initialized to prevent invalid mode changes*/\n"
				+ "\tint unicode;                         /*!< Flag to let us know we were initialized with a unicode string */\n"
				+ "\tCALLBACK_T callback;                 /*!< Callback for fips indicator*/\n"
				+ "\tTRACE_CALLBACK_T trace_callback;			  /*!< Tracecallback */\n"
				+ "};\n\n" + "typedef struct ICClib_t ICClib;\n\n");
	}
}

// FileType to write iccpkg_a.c

class File_ICCPKG_A_C extends FileType {

	File_ICCPKG_A_C() throws Exception {
		super(Filenum.ICCPKG_A_C);
		name = "../iccpkg/iccpkg_a.c";
		writer = new FileWriter(name);
		prefix = "ICC_";
	}

	public void Preamble() throws Exception {
		super.Preamble();
	}

	public void Body(ICCFunction func) throws Exception {
		super.Body(func);
	}

	public void Postamble() throws Exception {
		// In the case of ICC the enum table isn't in the "public headers" so
		// it goes into the start of the source file.
		writer.write(generateEnum());
		writeInitFunction(ICCencapsulator.METAPrefix);
		super.Postamble();
	}

}

// FileType to write iccpkg_a.h

class File_ICCPKG_A_H extends FileType {

	File_ICCPKG_A_H() throws Exception {
		super(Filenum.ICCPKG_A_H);
		name = "../iccpkg/iccpkg_a.h";
		writer = new FileWriter(name);

	}

	public void Preamble() throws Exception {
		prefix = "ICC_";
		pcbtype = ICCencapsulator.ICCPCB;

		super.Preamble();
		writer.write("/*! \\file icc_a.h\n" + "* Function prototypes for the ICC API (ICCSDK)\n"
				+ "* This file is autogenerated and should only be included via icc.h\n" + "*/\n");

		writer.write("\n#ifndef INCLUDED_ICC_A\n#define INCLUDED_ICC_A\n\n");

		// Write the defines for ICC_Init/ICC_InitW
		writeExtraHeaderStuffUnCon("", false);
		writeGSKitExtraHeaderStuff();
	}

	public void Body(ICCFunction func) throws Exception {
		// if there's a doxygen style comment for this function emit it
		if(! func.javaonly) {
			writer.write(func.GenComment(this));
			func.WriteFunction(writer, "ICC_", pcbtype);
			writer.write(";\n\n");
		}
	}

	public void Postamble() throws Exception {

		writer.write("\n#endif /*INCLUDED_ICC_A*/\n");

		super.Postamble();
	}

}
// FileType to write ../iccpkg/muppet.mk

class File_Muppet_mk extends FileType {

	File_Muppet_mk() throws Exception {
		super(Filenum.ICCPKG_A_H);
		name = "../iccpkg/muppet.mk";
		writer = new FileWriter(name);

	}

	public void Preamble() throws Exception {
		if (ICCencapsulator.oldICC == true) {
			writer.write("MUPPET\t=\t $(OLD_ICC)/iccsdk/$(ICCLIB)\n");
		} else {
			writer.write("MUPPET\t=\n");
		}
		if( ICCencapsulator.Prefix.indexOf("C") >= 0 ) {
			writer.write("IS_FIPS\t=\t1\n");
		} else {
			writer.write("IS_FIPS\t=\n");
		}
	}

	public void Body(ICCFunction func) throws Exception {

	}

	public void Postamble() throws Exception {

	}
}
// FileType to write ../icc_test/one.sh

class File_one_sh extends FileType {

	File_one_sh() throws Exception {
		super(Filenum.ICCPKG_A_H);
		name = "../icc_test/one.sh";
		writer = new FileWriter(name);

	}

	public void Preamble() throws Exception {
		if ((ICCencapsulator.oldICC == false) && (ICCencapsulator.Prefix.indexOf("C") >= 0 )) {
			writer.write("# Disable tests of GSkit-Crypto components\n#GSKIT=\"yes\"; export GSKIT\n");	
		} else {
			writer.write("# Enable tests of GSkit-Crypto components\nGSKIT=\"yes\"; export GSKIT\n");
		}
	}

	public void Body(ICCFunction func) throws Exception {

	}

	public void Postamble() throws Exception {

	}
}

// FileType to write gsk_wrap2.c
// This is the new "dual ICC" GSkit wrapper
//

class File_GSKWRAP2_C extends FileType
{
    private String WrapperCode = "";
    private Boolean FIPS = false;

    File_GSKWRAP2_C() throws Exception
    {
	super(Filenum.GSK_WRAP_C);
	pcbtype = ICCencapsulator.ICCPCB;
	name = "../iccpkg/gsk_wrap2_a.c";
	writer = new FileWriter(name);
    }
    public void Preamble() throws Exception
    {
	prefix = "ICC_";
	super.Preamble();
	// A bit confusing. These flags are for building GSkit-Crypto 
	// If it's a FIPS release. then there's no non-FIPS ICC available as a partner,
	// but the FIPS one is present
	if( ICCencapsulator.Prefix.indexOf("C") >= 0) { 
	    FIPS = true;
	    writer.write("#define HAVE_C_ICC\n");
	} else if(ICCencapsulator.oldICC == true) {
	    // Normal build, we have TWO ICC's present, FIPS and non-FIPS
	    writer.write("#define HAVE_C_ICC\n");
	    writer.write("#define HAVE_N_ICC\n");
	} else {
	    // non-FIPS build with no FIPS partner available
	    writer.write("#define HAVE_N_ICC\n");
	}
    writer.write("\n");
   }
	/**
     * emit the body of a relayed function call - GSkit style, which just calls the namespaced function
     * Note that some functions - hardwired into here - get special treatment,
     * either because of FIPS requirements, or to avoid memory leaks.
     * @param func function being generated
     */
    public void Body(ICCFunction func) throws Exception
    {
	// Note: This code "looks like" it belongs in ICCFunction, not in here
	// simply because it references so many data members from ICCFunction
	// HOWEVER: The bulk of the KNOWLEDGE embedded in the code generation
	// is FileType specific - so I've left well alone
	//

	// Remove the functions we create manually
	if(!func.name.equals("Init") && 
	   !func.name.equals("InitW") &&
	   !func.name.equals("SetValue") &&
	   !func.name.equals("Attach") &&
	   !func.name.equals("Cleanup")
	   ) {
	    // Generate a function prototype for the namespaced functions we'll be calling
	    // It may as well be in here as we can't include both variants of icc.h
	    func.WriteFunction(writer,ICCencapsulator.ICCPrefix,pcbtype); 
	    writer.write(";\n");
	    if(func.isLegacy(this)) {
			writer.write("#if defined(HAVE_C_ICC)\n");
		func.WriteFunction(writer,ICCencapsulator.ALT_ICCPrefix,pcbtype);
		writer.write(";\n");
			writer.write("#endif\n");
	    }
	    // Is documentation available for the entry point, if so add it so GSkit's doxygen can pick it up ?
	    writer.write(func.GenComment(this));
	    // writer.write("/* "+func.name+" */\n");
	    func.WriteFunction(writer,"ICC_",pcbtype);
	    writer.write("\n{");
	    // Now the body...
	    writer.write("\n\tWICC_CTX *wpcb = (WICC_CTX *)pcb;\n");
	    if (func.name.equals("GetValue") || 
		func.name.equals("GetStatus") ) {

		writer.write("\tif(NULL != status) {\n"+
			     "\t\tstatus->majRC = ICC_ERROR;\n"+
			     "\t\tstatus->minRC = ICC_NOT_INITIALIZED;\n"+
			     "\t\tstrncpy(status->desc,\"ICC is not initialized (gsk_wrap2.c)\",ICC_DESCLENGTH-1);\n"+
			     "\t}"
			     );
	    } 
	    writer.write("\n\tif(NULL != wpcb) {\n");			 
	    {  
		if(FIPS) {
		    writer.write("\t\tif(NULL != wpcb->Cctx) {\n");
		    {

			writer.write("\t\t\t");
			if (! func.returntype.equals("void")) {
			    writer.write("return ");
			}
			func.WriteCallingFunction(writer,ICCencapsulator.ICCPrefix,"(wpcb->Cctx");		
			writer.write("\n\t\t}\n");	
		    }
		    writer.write("\t}\n");
		} else {
		    /* Syntactic fluff, so we can map the indentation on generated code */
		    writer.write("\t\tif(NULL != wpcb->Nctx) {\n");
			writer.write("\t\t\t");
			if (! func.returntype.equals("void")) {
			    writer.write("return ");
			} 
			func.WriteCallingFunction(writer,ICCencapsulator.ICCPrefix,"(wpcb->Nctx");
			writer.write("\n\t\t}\n");
		    if(func.isLegacy(this)) {
				writer.write("#if defined(HAVE_C_ICC)\n");
			writer.write("\t\tif(NULL != wpcb->Cctx) {\n");
			{
			    
			    writer.write("\t\t\t");
			    if (! func.returntype.equals("void")) {
				writer.write("return ");
			    }
			    func.WriteCallingFunction(writer,ICCencapsulator.ALT_ICCPrefix,"(wpcb->Cctx");		
			    writer.write("\n\t\t}\n");	
			}
				writer.write("#endif\n");
		    } else {
			if( !func.returntype.equals("void") &&
			    func.returntype.indexOf("*") < 0) {
			    writer.write(
					 "\t\treturn ("+func.returntype+")ICC_NOT_IMPLEMENTED;\n");
			}
		    }
		    writer.write("\t}\n");
		}
	    }
	 
	    /* Default, error exit */
	    if ( func.returntype.equals("void")) {
		writer.write("\treturn;\n");
	    } else {
		if( func.returntype.indexOf("*") >= 0) {
		    writer.write("\treturn ("+func.returntype+")NULL;\n");
		} else {
		    writer.write("\treturn ("+func.returntype+")ICC_FAILURE;\n");
		}
	    }	    
	    writer.write("}\n\n");   
	}
    }
    public void Postamble() throws Exception
    {
		OS os = new OS();
		writeExtraGSKitInit();
		super.Postamble();
		os.write_GSKexports(ICCencapsulator.functionnames);
    }
 

    /**
     * Write the extra API entry points for gsk_wrap.c that don't get done any other way
     */
    void writeExtraGSKitInit() throws Exception
    {
    }
} // End gsk_wrap2.c
	
	
// FileType to write jcc_a.h
// This namespaces ICC_ calls to JCC_ calls and is an extra include used by the Java 
// variant of the step library

class File_JCC_A_H extends FileType {
	/* Note needs to be sort of in sync with the list in Class OS 
		These are functions that need namespacing
		OS contains the list of exports, slightly different
	*/
	private static String ExtraFuncs[] = {
		"Init",
		"GenerateRandomSeed",
		"GetValue",
		"HKDF",
		"HKDF_Expand",
		"HKDF_Extract",
		"MemCheck_start",
		"MemCheck_stop"
	};


	File_JCC_A_H() throws Exception {
		super(Filenum.JCC_A_H);
		name = "../iccpkg/jcc_a.h";
		writer = new FileWriter(name);

	}

	public void Preamble() throws Exception {

		pcbtype = ICCencapsulator.ICCPCB;

		writer.write("/*! \\file jcc_a.h\n" + "* Function prototypes for the ICC API (ICCSDK) - JCEPlus version \n"
				+ "* This file is autogenerated and should be included prior to icc.h\n" + "*/\n\n");
		// do this more elegantly iff we have more OS specific calls
		writer.write("#if defined(_WIN32)\n");
		writer.write("#  define ICC_InitW JCC_InitW\n");
		writer.write("#endif\n");
		for (String tfunc : ExtraFuncs) {
			writer.write("#define ICC_" + tfunc + " JCC_" + tfunc + "\n");

		}

	}
	public void Body(ICCFunction func) throws Exception {
		writer.write("#define ICC_"+func.name+" JCC_"+func.name+"\n");
	}

	public void Postamble() throws Exception {

		super.Postamble();
	}

} // End jcc_a.h

// FileType to write icc_aux_a.c

class File_ICC_AUX_A_C extends FileType
{

    File_ICC_AUX_A_C() throws Exception
    {
	super(Filenum.ICC_AUX_A_C);
	pcbtype = ICCencapsulator.ICCPCB;
	name = "../iccpkg/icc_aux_a.c";
	writer = new FileWriter(name);
    }
    public void Preamble() throws Exception
    {  
	writer.write("\n#if defined(ICC_AUX)\n");
	super.Preamble();	
	writer.write("\n#endif /* defined(ICC_AUX) */\n");
    }
    /**
     * @brief emit the body of a relayed function call.
     * Note that this looks different from the normal ICC
     * as we populate a fixed table at startup instead of in
     * the context as this can ONLY come from a non-FIPS context
     * @param myWriter output stream
     */
    public void emitFunctionBody(ICCFunction func) throws Exception
    {
	String pcb = "";

	func.WriteFunction(writer,prefix,pcbtype);
	// Start of function body
	writer.write("\n{\n");
	// Write tempory value for a return value if the called function
	// has one
	func.WriteRV_TEMP(writer);
	writer.write("\t"+func.typedefname+" tempf = NULL;\n");
	// Does this function need a PCB passed ?, if so check here that
	// wasn't NULL
	writer.write("\tif(NULL != funcs) {\n");
	// are swapped due to an error in another thread
	writer.write("\t\ttempf = ("+func.typedefname+")funcs["+ICCencapsulator.funcnum+"].func;\n");
	writer.write("\t}\n");
	// Write code for conditional tests executed before we call the function
	func.WriteBodyPreamble(writer,requiresicclibpcb && func.usespcb,func.genGlobal(this),genEnum(func.name));
	
	// set up to call the function indirectly via the pcb passed in
	if (passesicclibpcb && requiresiccpcb && func.usespcb) pcb = "(void*)pcb->funcs";
	func.WriteCallingFunctionBody(writer,pcb);
	// Terminate function call statement
	writer.write(";\n");
	// a dummy in the base class, but overridden is derived classes.
	writeAnyExtraCodeInFunction(func);
	writer.write("\t\t}\n");
	// Check that we don't try to return an error status
	// for functions with void return types
	if (requiresicclibpcb) {
	    func.WriteRVWarning(writer);
	}
	// 
	//	writer.write("\t}\n");
	func.WriteReturn(writer);
	// End of function
	writer.write("}\n\n");
	writeAnyExtraFunction(func);
    }
   /**
     * emit the body of a relayed function call - GSkit style, which just calls the namespaced function
     * Note that some functions - hardwired into here - get special treatment,
     * either because of FIPS requirements, or to avoid memory leaks.
     * @param func function being generated
     */
    public void Body(ICCFunction func) throws Exception
    {
	prefix = "ICC_";
	super.Body(func);
    }
    /** 
     * Generate the very large string for the enum tables used in ICC
     * We generate this as a string, as it must be generated by the pass
     * that creates icclib_a.c, but written into icclib_a.h
     * For icc_a.c/icc_a.h that problem doesn't arise, the enum table 
     * is private information and belongs in the C source.
     */
    String generateEnum() throws Exception 
    {
	String tmp; 
	tmp = "/*! @brief enum's for function table indices */\n"+
	    "/* Note: These get generated AFTER they could be used\n"+
	    "   in some of the code above (pcb-funcs[32].func) etc.\n"+
	    "   The problem is that'd require an extra pass as we don't\n"+
	    "   know all the values for this table until all functions\n"+
	    "   have been parsed. Since this is autogenerated (reliable) I\n"+
	    "   decided this was acceptable.\n"+
	    "*/\n";
	tmp += "typedef enum\n{\n";
	int j = ICCencapsulator.functionnames.size();
	for (int i = 0; i < j; i++) {
	    tmp += "\t" + genEnum(ICCencapsulator.functionnames.get(i)) + " = " + i +",\n";
        }
	tmp += "\t"+genEnum("TableEnd")+"\n";
	tmp += "} ICC_AUX_FUNCTION_ENUM;\n";
	return(tmp);
    }
    public void Postamble() throws Exception
    {
	int j = 0;
	for (String func: ICCencapsulator.functionnames) {
	    j++;
	}
	writer.write("\n#define NUM_ICC_AUXFUNCTIONS "+j+"\n\n");

	// writeStaticICCGlobalStructure("ICC_AUX");
	writer.write("\n/*! @brief this is the default data structure"+
		     "\n    that holds the call table for ICC_AUX"+
		     "\n*/"+
		     "\nstatic FUNC ICC_AUXGlobal_default[NUM_ICC_AUXFUNCTIONS] ="+
		     "\n{");
	for (String func: ICCencapsulator.functionnames) {
	    writer.write("\n\t{\"" + func +"\",NULL},");
	}
	writer.write("\n};\n");	
	ICCencapsulator.enumTable = generateEnum();
 	writer.write(ICCencapsulator.enumTable);
	writeInitFunction(ICCencapsulator.METAPrefix);
	
	super.Postamble();

    }
    // Suppress function prototypes in the C file
    public void emitHeader(ICCFunction func)
    {
    }


  /**
     * Generates the static header definition for the global/per-process ICCLib data
     * This is esentially the hooks into OpenSSL, which get populated once by the first
     * sucessfull load of the ICC libraries. 
     * Note that this may be de-populated as well !. 
     * For example: if the library signature check fails this structure will be invalidated
     * and we'll attempt to re-populate it again next time.
     */
    private void writeStaticICCGlobalStructure(String myPrefix) throws Exception
    {
	writer.write("\n/*! @brief this is the default data structure"+
		     "\n    that holds the call table for ICC"+
		     "\n*/"+
		     "\nstatic FUNC ICCGlobal_default[NUM_ICC_AUXFUNCTIONS] ="+
		     "\n{");
	for (String func: ICCencapsulator.functionnames) {
	    writer.write("\n\t{\"" + func +"\",NULL},");
	}
	writer.write("\n};\n");		      
    }

} // End icc_aux_a.c

// FileType to write icc_aux_a.h
class File_ICC_AUX_A_H extends FileType
{
    private static int first_done = 0;

    File_ICC_AUX_A_H() throws Exception
    {
	super(Filenum.ICC_AUX_A_H);
	pcbtype = ICCencapsulator.ICCPCB;
	name = "../iccpkg/icc_aux_a.h";
	writer = new FileWriter(name);

    }
    public void Preamble() throws Exception
    {
	super.Preamble();

	writer.write("/** \\file icc_a.h\n"+
		     "* Function prototypes for the ICC extended API.\n"+
		     "* This file is autogenerated and should only be included via icc_aux.h.\n"+
		     "*/\n\n");
	writer.write("\n#define NUM_NON_AUXFUNCTIONS "+ICCencapsulator.number_of_functions+"\n\n");

		
 	writer.write("#if !defined(ICC_AUX_H)\n");
   }

    public void Body(ICCFunction func) throws Exception
    {
	if(0 == first_done) {
	    writer.write("#define FIRST_AUX_NAME \""+func.name+"\"\n"); 
	    first_done = 1;
	}
	// if there's a doxygen style comment for this function emit it
	writer.write(func.GenComment(this));	
	func.WriteFunction(writer,"ICC_",pcbtype);
	writer.write(";\n\n");
    }   
    public void Postamble() throws Exception
    {	
	super.Postamble();
	writer.write("#endif /*!defined(ICC_AUX_H) */\n");

	OS os = new OS();
	os.write_AUXexports(ICCencapsulator.functionnames);	
    }
} // End icc_aux_a.h



// Encapsulates the knowledge of a function
class ICCFunction 
{
    //type of arguments - with type substitutions if needed for the filetype
    public String[] argumenttypes;
     //type of arguments - unmodified
    String[] privArgumenttypes;

    // name of arguments 
    String[] argumentnames;
    public boolean legacy = false;
   // name of the current parsed function
    public String name = ""; 
    // number of arguments
    public int numarguments;
    // apilevel tag for this function
    public int apilevel = 0;
    // Marker for "pcb" parameter if needed
    static final String PCB_TAG = "%PCB%";
    
    // generated typedef name, needed to generate type-safe indirect function calls
    public String typedefname = "";   
    // it's global because function like comment generation relies on this 
    public String returntype = ""; // The return type of the current parsed function - with modifier
 
    String privReturntype = ""; // The return type of the current parsed function, 
    
    // 'E' tag in functions.txt , this is also used in comment generation.
    public boolean errorsensitive = false;

    // 'F' tag in functions.txt, functions for which we need 'direct' access
    //  to the OpenSSL API without an ICC context being available.
    //  Generate internal functions/prototypes with a fixed "ef" prefix
    //  that dereference the global table directly.
    public boolean macrofunction = false;

    // 'P' tag in functions .txt, these functions pass the pointer to the ICC library handle
    // through the downstream call. (Most OpenSSL calls don't need to do this)
    public boolean usespcb = false;

    // 'M' tag in functions .txt, these functions are redirected to my_function
	 public boolean redirect = false;
	 // 'J' tag in functions.txt, only exported from the Java version of the step library, don't appear in headers
	 public boolean javaonly = false;
	 // 'C' tag in functions.txt, this function supports the FIPS callback function. Doc only for the code generator
	 //     Note: This usually requires the M tag to trap the function
	 public boolean FIPS_callback = false;

    public String comment = ""; // comment text for this function
    public boolean current = false; // instantiated in current functions.txt


    private String modifyerstring = ""; 

    public void dump()
    {
	System.out.println(returntype + " "+name+"\n"+
			   comment+"\n" );

    }
    public int parse(String s)
    {
        // We save 'doxygen' style comments and add them to generated output.
         if (s.charAt(0) == '#') {
	     HandleComment(s);
            return 1;
        }
        // If we got to here it better be a function prototype
        modifyerstring = s.substring(0,s.indexOf(' ')).trim();
	// Which is fine, but we can have ;'s within lines
	// a function definition starts with a digit.
	// so a wise person would avoid ;<digit> in comments in functions.txt ....
	if (!Character.isDigit(modifyerstring.charAt(0))) {
	    return 1;
	}
        apilevel = Character.digit(s.charAt(0),10); // Ignored now.


	    
	// E flag: We have to process this function on this pass, is it error ensitive ?
	// System.out.println("modifier = "+modifyerstring+"\n");
	  
	if (modifyerstring.indexOf('E') != -1 ) errorsensitive = true;

	// F flag: We need a macro'd pseuodofunction, it's called somewhere with no ICC context
	macrofunction = false;
	if (modifyerstring.indexOf('F') != -1 ) macrofunction = true;
	
	// P flag: We need to pass the icclib PCB through the call
	if (modifyerstring.indexOf('P') != -1 ) usespcb = true;

	// M flag: Modified function, public API is ICC_func(), called function is my_func() 
	//         This is used for the few functions where we need to intercept OpenSSL calls
	if (modifyerstring.indexOf('M') != -1) redirect = true;

	// U flag: Function is private to GSkit, it's pushed into the step library
	// but isn't exported from there and doesn't appear in the public headers.
	
	// J Java only function
	//
	if (modifyerstring.indexOf('J') != -1) javaonly = true;

	// C Supports the FIPS callback function indicating FIPS compliant algorithm/sizes
	//
	if (modifyerstring.indexOf('C') != -1) FIPS_callback = true;

	s = s.substring(s.indexOf(' '),s.length()).trim();


	
	StringTokenizer t = new StringTokenizer(s.substring(0,s.indexOf('('))," \t");
	privReturntype = t.nextToken();
	    
	name = t.nextToken();
	while (t.hasMoreTokens()) {
	    privReturntype = privReturntype + " " + name;
	    name = t.nextToken();
	}
	if (name.charAt(0) == '*') {
	    name = name.substring(1);
	    privReturntype = privReturntype + " *";
	}
	// Just a prefix that won't conflict with real function names we use 
        typedefname = "fptr_" + name;

	//type of argument
	argumenttypes = new String[20];
	//name of argument
	argumentnames = new String[20];
	    
	//type of argument
	privArgumenttypes = new String[20];

	    
	//NOTE: 'void' types have empty string types and 'void' as the name
	//NOT TRUE: when we are doing function pointers. 'void' can be the returntype
	    
	numarguments = 0;
	    
	StringTokenizer f = new StringTokenizer(s.substring(s.indexOf('(')+1),",");
	while (f.hasMoreTokens()) {
	    String nameandtype = f.nextToken();
	    StringTokenizer q = new StringTokenizer(nameandtype," ");
		
	    privArgumenttypes[numarguments] = "";//new String("");
		
	    String temp = q.nextToken();
		
	    if ( temp.equals(";") ) {
		//no more arguments
		break;
	    }
	    int numparens = 0;
	    while (q.hasMoreTokens()) {
		numparens = 0;
		    
		if (privArgumenttypes[numarguments] == null ) {
		    privArgumenttypes[numarguments] = temp;
		}
		else {
		    privArgumenttypes[numarguments] = privArgumenttypes[numarguments] + " " + temp;
		} 
		    
		temp = new String();
		do {
		    //System.out.println("temp: " + temp);
		    int basestringindex = temp.length();
		    int stringindex = basestringindex;
		    if (q.hasMoreTokens()) {
			if (temp.length() == 0 ) temp = q.nextToken();
			else temp = temp + " " + q.nextToken();
		    }
		    else if (f.hasMoreTokens()) temp = temp + "," + f.nextToken();
		    else {
			System.out.println("Mismatched Parenthesis\n");
			System.exit(-1);
		    }
		    //System.out.println("temp2: " + temp + "\n");
		    int parenloc = 0;
		    while (stringindex < temp.length() && parenloc != -1) {
			parenloc = temp.indexOf('(',stringindex);
			if (parenloc != -1 ) {
			    numparens++;
			    stringindex = parenloc+1;
			}
		    }
		    stringindex = basestringindex;
		    parenloc = 0;
		    while (stringindex < temp.length() && parenloc != -1) {
			parenloc = temp.indexOf(')',stringindex);
			if (parenloc != -1) {
			    numparens--;
			    stringindex = parenloc+1;
			}
		    }
		    //System.out.println("Num parens =" + numparens+ '\n');
		} while (numparens > 0);
	    }
		
	    //get rid of the extra space in the front
	    if (privArgumenttypes[numarguments].length() > 0) {
		privArgumenttypes[numarguments] = privArgumenttypes[numarguments].substring(1);
	    }	   
	    argumentnames[numarguments] = temp;
	    numarguments++;
		
	}
	    
	//now get rid of the extra paren on the end
	argumentnames[numarguments-1] = 
	    argumentnames[numarguments-1].substring(0,argumentnames[numarguments-1].length()-1);
	
	returntype = privReturntype;
	for(int i = 0; i < numarguments; i++) {
	    argumenttypes[i] = privArgumenttypes[i];
	}
	return 0;
    }
    // Check for whether we should emit extra comments to functions
    // not in the older ICC API
	private boolean legacyCheck(FileType fn) {
		boolean rv = false;
		switch (fn.fn) {
		case ICCPKG_A_H:
		case GSK_WRAP2_C:
		case JCC_A_H:
			if (!legacy)
				rv = true;
			break;
		default:
			break;
		}
		return rv;
	}

	public void putLegacy(FileWriter writer) throws Exception {
		if (legacy)
			writer.write("/* This function exists in the older ICC version */\n");
	}

	// tag that we have a legacy implementation of this class
	public void taggit() {
		legacy = true;
	}

	// Is the legacy function exposed in a particular file ?
	public boolean isLegacy(FileType fn) {
		return legacy && isMemberOf(fn);
	}

	// Test whether this function is relevant to a particular filetype
	public boolean isMemberOf(FileType filetype) {
		boolean rv = false;
		switch (filetype.fn) {
		case ICC_A_C:
		case ICCPKG_A_C:
			if (modifyerstring.indexOf('a') >= 0)
				return true;
			break;
		// Anything in a header, should have a corresponding export symbol
		case GSK_WRAP_C:
		case GSK_WRAP2_C:
			if (modifyerstring.indexOf('b') >= 0)
				return true;
			break;
		case ICC_A_H:
		case ICCPKG_A_H:
		case JCC_A_H:
			if (modifyerstring.indexOf('b') >= 0)
				return true;
			break;
		case ICCLIB_A_C:
			if (modifyerstring.indexOf('c') >= 0)
				return true;
			break;
		case ICCLIB_A_H:
			if (modifyerstring.indexOf('d') >= 0)
				return true;
			break;
		case ICC_AUX_A_C:
			if (modifyerstring.indexOf('e') >= 0)
				return true;
			break;
		case ICC_AUX_A_H:
			if (modifyerstring.indexOf('f') >= 0)
				return true;
			break;
		default:
			break;
		}
		return rv;
	}
	public void fixTypes(FileType filetype)
	{
	//okay, now we have the function parsed, we need to output	
	// Go through our variable types and prepend anything with EVP/RSA etc in it with ICC_
	// if we need to
	int k =  PrependWords.words().size();

	switch (filetype.fn) {
		case ICC_A_C:
		case ICC_A_H:
		case ICCPKG_A_C:
		case ICCPKG_A_H:
		case GSK_WRAP_C:
			int loc;
			String beginning;

			for (int i = 0; i < numarguments; i++) {
				for (int j = 0; j < k; j++) {
					loc = privArgumenttypes[i].indexOf((String) PrependWords.words().get(j));
					if (loc != -1) {
						if (loc != 0)
							beginning = privArgumenttypes[i].substring(0, loc);
						else
							beginning = new String();
						// Note: Don't use ICCPrefix here, data types aren't namespaced.
						argumenttypes[i] = beginning + "ICC_" + privArgumenttypes[i].substring(loc);
						break;
					}
				}
			}
			for (int j = 0; j < k; j++) {
				loc = privReturntype.indexOf((String) PrependWords.words().get(j));
				if (loc != -1) {
					if (loc != 0)
						beginning = privReturntype.substring(0, loc);
					else
						beginning = new String();
					// Note: Don't use ICCPrefix here, data types in return values aren't
					// namespaced.
					returntype = beginning + "ICC_" + privReturntype.substring(loc);
					break;
				}
			}
			break;
		default:
			for (int j = 0; j < numarguments; j++) {
				argumenttypes[j] = privArgumenttypes[j];
			}
			returntype = privReturntype;
			break;
	}

}

    // Write the body of a function call 
    public void WriteCallingFunctionBody(FileWriter writer,String pcbtype) throws Exception
    {
	if(pcbtype != "") {
	    writer.write(pcbtype);
	    if(! argumentnames[0].equals("void")) writer.write(",");
	}
	// function parameters
	for (int i = 0; i < numarguments;i++) {
	    if (! argumentnames[i].equals("void")) {
		if (i != 0) {
		    writer.write(",");
		}
		int numstars = 0;
		while (argumentnames[i].substring(numstars).startsWith("*")) {
		    numstars++;
		}
		if (! argumentnames[i].equals("void")) {
		    String strtmp = argumentnames[i].substring(numstars);
		    if (strtmp.startsWith("(*")) { //we have a function pointer
			strtmp = argumentnames[i].substring(2,argumentnames[i].indexOf(')'));
		    }
		    writer.write(strtmp);
		}
	    }
	}
	writer.write(")");	
    }
    ////////////////////////////////////////////////////////////////////////////
    // Code for writing small parts of functions
    ////////////////////////////////////////////////////////////////////////////

    // Write a temporary variable to hold a return value if the called function has one.
    public void WriteRV_TEMP(FileWriter writer) throws Exception
    {
    	if (! returntype.equals("void")) writer.write("\t" + returntype + " temp = "+ genReturnValue());
    }
    //
    // Write the start of an indirect function call i.e. ( (typedef)func) ( 
    //
    public void WriteIndirectCall(FileWriter writer,String global) throws Exception
    {
	writer.write("(tempf)(");
    }

    // Write the code leading up to the function call
    public void WriteBodyPreamble(FileWriter writer,boolean pcb,String global ,String enumName)  throws Exception
    {
	// If it's FIPS sensitive AND there's a pcb to check add checking code
	if (errorsensitive && pcb ) {
	    writer.write(" && !((pcb->flags & ICC_FIPS_FLAG) && error_state)\n");
	}
	writer.write("\t\tif( NULL != tempf ) {\n");			
	if (returntype.equals("void")) {
	    writer.write("\t\t\t");
	} else {              
	    writer.write("\t\t\ttemp = ");
	}
	WriteIndirectCall(writer,global);
    }
    // Check that we don't try to return an error status
    // for functions with void return types
    public void WriteRVWarning(FileWriter writer) throws Exception
    {
	if (errorsensitive && returntype.equals("void")) {
	    System.out.println("Warning. "+ name + ": This interface has a void return\n"+
			       "marking it Error sensitive will result in silent failures\n");
	}	                 

    }
    // write temp = if there's a return type - otherwise - don't
    public void WriteReturnEQ(FileWriter writer) throws Exception
    {
	if (returntype.equals("void")) {
	    writer.write("\t");
	} else {              
	    writer.write("\ttemp = ");
	    
	}
    }
    // Write the code to return a value (if there is one)
    public void WriteReturn(FileWriter writer) throws Exception
    {
	if (returntype.equals("void")) {
	    writer.write("\treturn;\n");
	} else {
	    writer.write("\treturn temp;\n");
	}
    }
    
    // Write a function with no extra parameters - these are typically callback's from OpenSSL
    public void WriteMacroFunction(FileWriter writer) throws Exception
    {		
	writer.write(returntype + " ");
	writer.write(gen_ef_Function()+"(");
	for (int i = 0; i < numarguments;i++) {
	    if (! argumentnames[i].equals("void") ) {
		if ( i != 0) {
		    writer.write(",");
		}
		writer.write(argumenttypes[i]+" "+argumentnames[i]);
	    } else {
		writer.write("void");
		break;
	    }
	}
	writer.write(")");
    }	
    // Write a typedef prototype for an indirect call
    public void WriteTypedef(FileWriter writer,boolean haspcb) throws Exception
    {
	writer.write("typedef " + returntype + " (*" + typedefname + ")(");
	if (haspcb) writer.write("void *pcb");
	for (int i = 0; i < numarguments;i++) {
	    if (! argumentnames[i].equals("void")) {
		if (i!=0 || haspcb) {
		    writer.write(",");
		}
		writer.write(argumenttypes[i]+" "+argumentnames[i]);
	    } else if(! haspcb) {
		writer.write("void");
	    }
	}
	writer.write(");\n");
    }

    // Write a namespaced function
    public void WriteFunction(FileWriter writer,String prefix, String pcbtype) throws Exception
    {
	writer.write(returntype + " ");
	if(pcbtype == ICCencapsulator.ICCPCB) {
	    writer.write("ICC_LINKAGE ");
	}
	writer.write(prefix+name);
	if(pcbtype != "") {
	    writer.write("("+pcbtype);
	    if (!argumentnames[0].equals("void")) {
		writer.write(",");
	    }
	} else {
	    writer.write("(");
	}
	if (argumentnames[0].equals("void") && ("" == pcbtype) ) {
	    writer.write("void");
	} else {
	    for(int i = 0; i < numarguments;i++) {
		if (! argumentnames[i].equals("void") ) {
		    if (i != 0)  {
			writer.write(",");
		    }
		    writer.write(argumenttypes[i]+" "+argumentnames[i]);
		} 
	    }
	}
	writer.write(")");	
    }
    // Write a function as a caller i.e. no parameter types
    public void WriteCallingFunction(FileWriter writer,String prefix,String pcbtype) throws Exception
    {
	// namespaced function name
	writer.write(prefix + name);
	WriteCallingFunctionBody(writer,pcbtype);
	writer.write(";");
    }


    /**
     * Handle inline doxygen style comments in functions.txt
     * This allows us to produce adequately commented autogenerated code.
     * These comments are marked with #! and are cleared when a non-doxygen
     * style comment is encountered. 
     * "#;" is typical for a 'clear comment' marker
     * Note: There are cases where we DO reuse comments.
     * These comments must precede the function the comment is relevant to.
     * 'typical' pattern would be
     * #; 
     * #! @brief This is a brief description of the function following;
     * #! @param status pre-allocated ICC_STATUS to return error status;
     * #! @return int, ICC_OK , ICC_ERROR or ICC_WARNING;
     * 0abdE   int                     Attach(ICC_STATUS* status);
     * #;
     * 
     * Note comments for the ICC_CTX type of parameter (auto-inserted)
     * are generated after @brief is found.
     * The data is stored for a while and eventually retrieved by genComment()
     * @param s Input line from functions.txt which "looked like" a comment tag.
     */
    public void HandleComment(String s) 
    {
        String tmp = "";
        // System.out.println("<"+s+"> ");
        // Does it look like a doxygen marker ?
        if(s.length() >2 && s.charAt(1) == '!') {  
            /*
              go to extreme lenths to pretify the output
              If this comment was @brief, and we'll insert a tag so we know
	      where to create the @param for auto-inserted fields i.e. "pcb"

            */
	    if( (comment.indexOf("@brief") > 0) && 
 		(s.indexOf("@param") > 0) && 
 		(comment.indexOf("@param") < 0) ) {
		comment += PCB_TAG;	 
	    }
	    // top and tail the string, reformat as doxygen style (minimal)
            // append to comment
            tmp = s.substring(2);
            comment = comment + " * " + tmp +"\n";
	    // System.out.println("<"+comment+"> ");
        } else {
	    comment = "";
	}
    }
    // substitute one tag in a string with replacement text
    private String Subst(String s, String old, String newS)
    {
	String [] sa1;
	sa1 = s.split(old,2);
	if( sa1.length > 1) {
	    s = sa1[0] + newS + sa1[1];
	}
	return s;
    }
    // As for subst, except do the replacement starting at an offset into the string
    private String Subst(String s, int offset, String target, String replacement)
    {
	String start = new String("");
	if(offset > 0) {
	    start = s.substring(0,offset);
	    s = s.substring(offset);
	}
	s = start + Subst(s,target,replacement);
	return s;
    }
    /**
     * @brief convert all references to OpenSSL types in the comment
     * into the appropriate ICC type
     * @param tmp the input string
     * @return the processed comment
     */
    private String SubstICCTypes(String tmp) {
        String tmp1;
        String tmp2;
	int i,k;
	k =  PrependWords.words().size();
	for (int j = 0; j < k; j++) {	
	    tmp1 = (String)PrependWords.words().get(j);
	    tmp2 = " "+tmp1;			
	    while ( (i = tmp.lastIndexOf(tmp2)) > 0 ) {
		String t1;
		String t2;
		t1 = tmp.substring(0,i+1);
		t2 = tmp.substring(i+1);
		tmp = t1 + "ICC_" + t2;
	    }
	}
	return tmp;
    }
    /**
     * @brief If a doxygen style comment has been parsed, 
     * convert it to create a doxygen style  comment in the output.
     * Some assembly and text substitution required.
     * @param filetype the file context for the operation
     * @return A massaged version of the comment text originally parsed from functions.txt
     * @note as we use indirect function calls it's quite hard to trace
     * down the code using Doxygen generated data, so we add references
     * to applicable upstream and downstream functions 
     * - well that's the evil plan for world domination anyway.
     */
	public String GenComment(FileType filetype) {
		String tmp = new String("");

		if (comment != "") {
			tmp = "/*!\n" + comment; // Add start of comment
			/*
			 * If it's one of the ICC API files generated, make sure we convert comments to
			 * generate ICC_ variant of names This does require a little care in writing the
			 * docs, but it'll mostly work painlessly
			 */
			switch (filetype.fn) {
			case ICC_A_C:
			case ICC_A_H:
				// Insert a cross reference to the downstream function
				tmp += " *\n * <b>Indirect call to:</b> \\ref " + name + "()\n";
				// Convert OpenSSL types in comments to ICC_ types
				// No longer tmp = SubstICCTypes(tmp);
				if(FIPS_callback) {
					tmp += " *\n * @note this function supports the FIPS algorithm callback function";
					tmp += " \n * IF FIPS is enabled and the callback has been set in the ICC_CTX";
					tmp += " \n * a 1 will be returned by the callback prior to return for a FIPS algorithm properly configured, 0 otherwise\n";
				}
				break;
			case ICCPKG_A_C:
			case GSK_WRAP_C:
			case GSK_WRAP2_C:
				tmp = SubstICCTypes(tmp);
				break;
			case ICCLIB_A_C:
			case ICCLIB_A_H:
				// Insert a reference to the downstream (OpenSSL) function
				tmp += " *\n * <b>Indirect call to:</b> \\ref " + name + "()\n";
				break;
			default:
				break;
			}
			// Now deal with the comment for the "pcb" parameter that the ICC API inserts
			// We shoved a tag into the saved comment text in the appropriate place when we
			// read it
			switch (filetype.fn) {
			case ICC_A_C:
			case ICC_A_H:
			case ICCPKG_A_H:
			case ICCPKG_A_C:
			case GSK_WRAP_C:
			case GSK_WRAP2_C:
				tmp = Subst(tmp, PCB_TAG,
						" *  @param pcb ICC context pointer returned by a sucessful call to ICC_Init\n");

				break;
			case ICCLIB_A_C:
			case ICCLIB_A_H:
			case JCC_A_H:
				tmp = Subst(tmp, PCB_TAG,
						" *  @param pcb OpenSSL Library context pointer. This parameter is never exposed in public API's\n");
				break;
			default:
				tmp = Subst(tmp, PCB_TAG, "");
				break;
			}

			// If there's an @return line, check for extra processing
			if (tmp.indexOf("@return") > 0) {
				tmp = Subst(tmp, "@return", "@return" + genExtraRVComment(filetype));
			}
			// Add warning if the API won't be in the older ICC
			if (legacyCheck(filetype)) {
				tmp = tmp + " *  @note WARNING! This function is not implemented by all ICC contexts.\n";
			}
			tmp = genExtraComment(tmp, filetype);
			tmp = tmp + "*/\n"; // add end of comment
		}
		return tmp;
	}
    
    /**
     * Generate extra comment text at the @return phase
     * if it does document the possible failure causes.
     * i.e. add documentation of the ICC API failure modes..
     * @return the extra return text
     */  
    public String genExtraRVComment(FileType filetype)  
    { 
        int i1, i2;
        String tmp = new String("");

	if (filetype.requiresiccpcb  || filetype.requiresicclibpcb ) {
	    if(! returntype.equals("void") ) {
		if( returntype.indexOf("*") == -1) {
		    if( errorsensitive ) {
			tmp += 
			    "\n *  ICC_FAILURE if a FIPS mode error occured,";
		    }
		    if(legacyCheck(filetype) ) {
			tmp += 
			    "\n *  ICC_NOT_IMPLEMENTED if not supported by an older ICC instance,";

		    }
		} else {
		    if(errorsensitive) {
			tmp += 
			    "\n *  NULL if a FIPS mode error occured,";
		    }	
		    if(legacyCheck(filetype) ) {
			tmp += 
			    "\n *  NULL if the API is not supported by an older ICC instance,";
		    }
		}
	    }
	}
	return tmp;
    }
    public String genExtraComment(String s,FileType filetype)  
    {
	int i1,i2;
	i1 = s.indexOf("@return");
	// See if we reference any of the ICC error enum values in @return
	if(( (s.indexOf("ICC_OSSL_FAILURE",i1) >0) ||
	     (s.indexOf("ICC_OSSL_OK",i1) > 0) || 
	     (s.indexOf("ICC_FAILURE",i1) > 0) ||
	     (s.indexOf("ICC_NOT_IMPLEMENTED",i1) > 0) 
	     ) 
	   )  {  // If there were, cross reference this
	    i2 = s.lastIndexOf("\n");
	    s = Subst(s,i2,"\n","\n *  @see ICC_RC_ENUM\n");
	}
	return s;
    }

    /**
     * Returns function names with/without extra modifiers - implementation context
     * @param func raw function name from functions.txt
     * @return Function name modified with prefixes depending on where it's being used
     */ 
    public String genHasFunction(FileType filetype)
    {
        String rv = new String("");
        if (filetype.hasICCprefix == true ) rv = ICCencapsulator.ICCPrefix + name;
        else if (filetype.hasMETAprefix == true ) rv= ICCencapsulator.METAPrefix + name;
        else rv = name;
        return rv;
    }

    /**
     * Returns function names with an "ef" prefix.
     * these are functions which are called during startup, or called back via OpenSSL
     * used internally within icclib level code only.
     * @param func raw function name from functions.txt
     * @return Function name modified with prefixes depending on where it's being used
     */ 
    public String gen_ef_Function()
    {
        return new String("ef"+name);
    }



    /**
     * Generates a string which is name of the global structure holding
     * shared library information/call vectors 
     * for either the ICC or OpenSSL libraries
     * @param func The root function name
     * @return The input converted to an enum
     */
    public String genGlobal(FileType filetype)
    {
        String rv = new String("");
        if (filetype.hasICCprefix == true) rv = "ICCGlobal";
        else if (filetype.hasMETAprefix == true) rv= "Global";
        else rv = "__BROKEN__";
        return rv;

    }
    /**
     * Generate the return data for an ICC API error return.
     * @return  if the return type is "void" nothing
     *          else if the return type is a pointer, return NULL
     *          else return (type)ICC_FAILURE
     */
    public String genReturnValue() {
        String rv = new String(";\n");
        if (! returntype.equals("void")) {
            if (returntype.indexOf('*') == -1) {
                rv = " (" + returntype + ")ICC_FAILURE;\n";
            }
            else {
                rv = " NULL;\n";
            }		    
        }
        return rv;
    }

} 


// Encapsulates knowledges specific to a supported OS
class OS {
	// Extra symbols required to be exported from the file GSkit creates
	private static String GSKExports[] = { 
		"C_GetFunctionList",
		"gskiccs_SCCSInfo", 
		"gskiccs_Crypto_VersionInfo", 
		"gskiccs_path", 
		"gskiccs8_path",
		"ICC_Init",
		"Delta_T", 
		"Delta_res", 
		"Delta2Time", 
		"Delta_spanT", 
		"Delta_spanC",
		"ICC_MemCheck_start", 
		"ICC_MemCheck_stop" /*, 
		"gsk_exp_init", 
		"ICC_newTotpCtx",
		"ICC_freeTotpCtx",
		"ICC_signTotp",
		"ICC_verifyTotp"*/
	};

	// Strings that need special handling for this file. Note no P11 i/f
	private static String JGSKExports[] = {
		"JCC_Init", 
		"JCC_HKDF", 
		"JCC_MemCheck_start", 
		"JCC_MemCheck_stop" /* ,
		"jgsk_exp_init"		*/
	};

	private static String ICKCExports[] = {
		"ICKC_Init", 
	};

	private static String AUXExports[] = { "AUX_Init", "AUX_Cleanup" };

	// Windows only exported symbols. I did consider making this per-OS, but
	// currently
	// only Windows has this issue.
	private static String GSKWinExports[] = { "ICC_InitW","gskiccs8_pathW","gskiccs_pathW" };
	private static String JGSKWinExports[] = { "JCC_InitW" };
	private static String ICKCWinExports[] = { "ICKC_InitW" };

	// Extra exported symbols, used by our FVT code calling into OpenSSL
	private static String OSSLExports[] = { "CRYPTO_num_locks" };
	private static String ICCLIBExports[] = { "lib_init" };

	// This enum defines the TYPE of export file processing to be done
	private enum OSTYPE {
		AIX, SUN, HP, WIN, LINUX, OS400, ZOS, OSX, OS2
	};

	private class ExportMe {
		private String fname;
		private OSTYPE os;

		ExportMe(String fname, OSTYPE os) {
			this.fname = fname;
			this.os = os;
		}

		public String fname() {
			return this.fname;
		}

		public OSTYPE os() {
			return this.os;
		}

	};

	private List<ExportMe> ICCExport;
	private List<ExportMe> GSKExport;
	private List<ExportMe> JGSKExport;
	private List<ExportMe> ICKCExport;
	private List<ExportMe> AUXExport;

	OS() {

		// ICC exports
		ICCExport = new ArrayList<ExportMe>();

		ICCExport.add(new ExportMe("icclib_win32.def", OSTYPE.WIN));
		ICCExport.add(new ExportMe("icclib_sun.exp", OSTYPE.SUN));
		ICCExport.add(new ExportMe("icclib_linux.exp", OSTYPE.LINUX));
		ICCExport.add(new ExportMe("icclib_aix.exp", OSTYPE.AIX));
		ICCExport.add(new ExportMe("icclib_hpux.exp", OSTYPE.HP));
		ICCExport.add(new ExportMe("icclib_os2.def", OSTYPE.OS2));
		ICCExport.add(new ExportMe("icclib_osx.def", OSTYPE.OSX));
		ICCExport.add(new ExportMe("icclib_os400.exp", OSTYPE.OS400));
		ICCExport.add(new ExportMe("icclib_zos.h", OSTYPE.ZOS));

		// GSkit exports
		GSKExport = new ArrayList<ExportMe>();

		GSKExport.add(new ExportMe("iccstepaix4.exp", OSTYPE.AIX));
		GSKExport.add(new ExportMe("iccstepsun64.exp", OSTYPE.SUN));
		GSKExport.add(new ExportMe("iccstepaix64.exp", OSTYPE.AIX));
		GSKExport.add(new ExportMe("iccstepsun64_x86.exp", OSTYPE.SUN));
		GSKExport.add(new ExportMe("iccstephpux.exp", OSTYPE.HP));
		GSKExport.add(new ExportMe("iccstepsun_x86.exp", OSTYPE.SUN));
		GSKExport.add(new ExportMe("iccstephpux64.exp", OSTYPE.HP));
		GSKExport.add(new ExportMe("iccstepwin.def", OSTYPE.WIN));
		GSKExport.add(new ExportMe("iccstephpux64_ia64_gcc.exp", OSTYPE.HP));
		GSKExport.add(new ExportMe("iccstepwin64.def", OSTYPE.WIN));
		GSKExport.add(new ExportMe("iccstephpux_ia64.exp", OSTYPE.HP));
		GSKExport.add(new ExportMe("iccsteplinux.exp", OSTYPE.LINUX));
		GSKExport.add(new ExportMe("iccstepsun4-sol2.exp", OSTYPE.SUN));
		GSKExport.add(new ExportMe("iccstepOS400.exp", OSTYPE.OS400));
		GSKExport.add(new ExportMe("iccstepZOS.h", OSTYPE.ZOS));
		GSKExport.add(new ExportMe("iccstepOSX.def", OSTYPE.OSX));

		// JGSkit exports
		JGSKExport = new ArrayList<ExportMe>();

		JGSKExport.add(new ExportMe("jccstepaix4.exp", OSTYPE.AIX));
		JGSKExport.add(new ExportMe("jccstepsun64.exp", OSTYPE.SUN));
		JGSKExport.add(new ExportMe("jccstepaix64.exp", OSTYPE.AIX));
		JGSKExport.add(new ExportMe("jccstepsun64_x86.exp", OSTYPE.SUN));
		JGSKExport.add(new ExportMe("jccstephpux.exp", OSTYPE.HP));
		JGSKExport.add(new ExportMe("jccstepsun_x86.exp", OSTYPE.SUN));
		JGSKExport.add(new ExportMe("jccstephpux64.exp", OSTYPE.HP));
		JGSKExport.add(new ExportMe("jccstepwin.def", OSTYPE.WIN));
		JGSKExport.add(new ExportMe("jccstephpux64_ia64_gcc.exp", OSTYPE.HP));
		JGSKExport.add(new ExportMe("jccstepwin64.def", OSTYPE.WIN));
		JGSKExport.add(new ExportMe("jccstephpux_ia64.exp", OSTYPE.HP));
		JGSKExport.add(new ExportMe("jccsteplinux.exp", OSTYPE.LINUX));
		JGSKExport.add(new ExportMe("jccstepsun4-sol2.exp", OSTYPE.SUN));
		JGSKExport.add(new ExportMe("jccstepOS400.exp", OSTYPE.OS400));
		JGSKExport.add(new ExportMe("jccstepZOS.h", OSTYPE.ZOS));
		JGSKExport.add(new ExportMe("jccstepOSX.def", OSTYPE.OSX));

		// ICKCkit exports
		ICKCExport = new ArrayList<ExportMe>();
		 
		ICKCExport.add(new ExportMe("ickcstepaix4.exp", OSTYPE.AIX));
		ICKCExport.add(new ExportMe("ickcstepsun64.exp", OSTYPE.SUN));
		ICKCExport.add(new ExportMe("ickcstepaix64.exp", OSTYPE.AIX));
		ICKCExport.add(new ExportMe("ickcstepsun64_x86.exp", OSTYPE.SUN));
		ICKCExport.add(new ExportMe("ickcstephpux.exp", OSTYPE.HP));
		ICKCExport.add(new ExportMe("ickcstepsun_x86.exp", OSTYPE.SUN));
		ICKCExport.add(new ExportMe("ickcstephpux64.exp", OSTYPE.HP));
		ICKCExport.add(new ExportMe("ickcstepwin.def", OSTYPE.WIN));
		ICKCExport.add(new ExportMe("ickcstephpux64_ia64_gcc.exp", OSTYPE.HP));
		ICKCExport.add(new ExportMe("ickcstepwin64.def", OSTYPE.WIN));
		ICKCExport.add(new ExportMe("ickcstephpux_ia64.exp", OSTYPE.HP));
		ICKCExport.add(new ExportMe("ickcsteplinux.exp", OSTYPE.LINUX));
		ICKCExport.add(new ExportMe("ickcstepsun4-sol2.exp", OSTYPE.SUN));
		ICKCExport.add(new ExportMe("ickcstepOS400.exp", OSTYPE.OS400));
		ICKCExport.add(new ExportMe("ickcstepZOS.h", OSTYPE.ZOS));
		ICKCExport.add(new ExportMe("ickcstepOSX.def", OSTYPE.OSX));

		// AUX exports
		AUXExport = new ArrayList<ExportMe>();
		AUXExport.add(new ExportMe("iccauxaix4.exp", OSTYPE.AIX));
		AUXExport.add(new ExportMe("iccauxsun64.exp", OSTYPE.SUN));
		AUXExport.add(new ExportMe("iccauxaix64.exp", OSTYPE.AIX));
		AUXExport.add(new ExportMe("iccauxsun64_x86.exp", OSTYPE.SUN));
		AUXExport.add(new ExportMe("iccauxhpux.exp", OSTYPE.HP));
		AUXExport.add(new ExportMe("iccauxsun_x86.exp", OSTYPE.SUN));
		AUXExport.add(new ExportMe("iccauxhpux64.exp", OSTYPE.HP));
		AUXExport.add(new ExportMe("iccauxwin.def", OSTYPE.WIN));
		AUXExport.add(new ExportMe("iccauxhpux64_ia64_gcc.exp", OSTYPE.HP));
		AUXExport.add(new ExportMe("iccauxwin64.def", OSTYPE.WIN));
		AUXExport.add(new ExportMe("iccauxhpux_ia64.exp", OSTYPE.HP));
		AUXExport.add(new ExportMe("iccauxlinux.exp", OSTYPE.LINUX));
		AUXExport.add(new ExportMe("iccauxsun4-sol2.exp", OSTYPE.SUN));
		AUXExport.add(new ExportMe("iccauxOS400.exp", OSTYPE.OS400));
		AUXExport.add(new ExportMe("iccauxZOS.h", OSTYPE.ZOS));
		AUXExport.add(new ExportMe("iccauxOSX.def", OSTYPE.OSX));

	};

	/**
	 * Write the various exports files for the OS's we support. This routine simply
	 * opens the appropriate file, writeDefFile does the formatting
	 */
	public void write_ICCexports(String prefix, List<String> functionlist) throws Exception {
		for (ExportMe Ex : ICCExport) {
			FileWriter myWriter = new FileWriter("exports/" + Ex.fname());
			// Note functionlist isn't used
			if (  ICCencapsulator.Prefix.indexOf("C") < 0 ) {
				writeICCDefFile(myWriter, Ex.os(), "N_", functionlist);
			} else {
				writeICCDefFile(myWriter, Ex.os(), "C_", functionlist);
			}	
			myWriter.close();
		}
	}

	/**
	 * Write the various exports files for the OS's GSKit support. This routine
	 * simply opens the appropriate file, writeGSKDefFile/writeJGSKDefFile does the formatting
	 */
	public void write_GSKexports(List<String> functionlist) throws Exception {
		for (ExportMe Ex : GSKExport) {
			FileWriter myWriter = new FileWriter("../iccpkg/exports/" + Ex.fname);
			writeGSKDefFile(myWriter, Ex.os(), functionlist, "ICCSTUB", "ICC_");
			myWriter.close();
		}
		for (ExportMe Ex : GSKExport) {
			FileWriter myWriter = new FileWriter("../iccpkg/exports_old/" + Ex.fname);
			writeGSKDefFile(myWriter, Ex.os(), functionlist, "GSKICCS", "ICC_");
			myWriter.close();
		}
		for (ExportMe Ex : JGSKExport) {
			FileWriter myWriter = new FileWriter("../iccpkg/exports/" + Ex.fname);
			writeJGSKDefFile(myWriter, Ex.os(), functionlist, "JGSKICCS", "JCC_");
			myWriter.close();
		}
		for (ExportMe Ex : ICKCExport) {
			FileWriter myWriter = new FileWriter("../iccpkg/exports/" + Ex.fname);
			writeICKCDefFile(myWriter, Ex.os(), functionlist, "ICKCICCS", "ICKC_");
			myWriter.close();
		}
	}

	/**
	 * Write the various exports files for the OS's GSKit support. This routine
	 * simply opens the appropriate file, writeGSKDefFile does the formatting
	 */
	public void write_AUXexports(List<String> functionlist) throws Exception {
		for (ExportMe Ex : AUXExport) {
			FileWriter myWriter = new FileWriter("../iccpkg/exports/" + Ex.fname);
			writeAUXDefFile(myWriter, Ex.os(), "ICC_", functionlist);
			myWriter.close();
		}
	}

	/**
	 * Write a properly formatted exports file for various OS's The exported
	 * functions are the ones currently in the functionnames list
	 * 
	 * @param myWriter The output stream
	 * @param osnum    The OS type to process.
	 */
	static void writeICCDefFile(FileWriter myWriter, OSTYPE os, String prefix, List<String> functionlist)
			throws Exception {
		switch (os) {
		case WIN:
			myWriter.write("DESCRIPTION 'ICCLIB EXPORT FILE'\n\nEXPORTS\n");
			for (String name : ICCLIBExports) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case AIX:
			myWriter.write("#!\n*DESCRIPTION 'ICCLIB EXPORT FILE'\n\n");
			for (String name : ICCLIBExports) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case SUN:
		case LINUX:
			myWriter.write("#DESCRIPTION 'ICCLIB EXPORT FILE'\n\nICCLIB {\n  global:\n");
			for (String name : ICCLIBExports) {
				myWriter.write("    " + prefix + name + ";\n");
			}
			myWriter.write("  local:\n    *;\n};");
			break;
		case HP:
			myWriter.write("#DESCRIPTION 'ICCLIB EXPORT FILE'\n\n");
			for (String name : ICCLIBExports) {
				myWriter.write("+e " + prefix + name + "\n");
			}
			myWriter.write("+e " + "icclib085_loaded_from" + ICCencapsulator.ICC_Version + "\n");
			break;
		case OS2:
			myWriter.write("LIBRARY         icclib  INITINSTANCE\n");
			myWriter.write("DATA NONSHARED\n\n");
			myWriter.write("DESCRIPTION     'ICC Shared Library'\n\n");
			myWriter.write("EXPORTS\n");
			for (String name : ICCLIBExports) {
				myWriter.write("\t_" + prefix + name + "\n");
			}
			break;
		case OSX:
			for (String name : ICCLIBExports) {
				// we need the '_' prepended here
				myWriter.write("_" + prefix + name + "\n");
			}
			break;
		case OS400:
			myWriter.write("STRPGMEXP PGMLVL(*CURRENT) SIGNATURE(\"LIBICCLIB\")\n");
			for (String name : ICCLIBExports) {
				// we need the '_' prepended here
				myWriter.write("EXPORT SYMBOL(\"" + prefix + name + "\")\n");
			}
			myWriter.write("ENDPGMEXP\n");
			break;
		case ZOS:
			myWriter.write("/* z/OS pragma's to control symbol visbility */\n\n");
			myWriter.write("#ifdef __cplusplus\n");
			myWriter.write("extern \"C\" {\n");
			myWriter.write("#endif\n\n");
			for (String name : ICCLIBExports) {
				// we need the '_' prepended here
				myWriter.write("#pragma export(" + prefix + name + ")\n");
			}
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("};\n");
			myWriter.write("#endif\n");
			break;
		}
	}

	/**
	 * @brief Writes the main list of exported functions, filtering those we don't want exported from the step library
	 * @param myWriter the output stream
	 * @param prefix Prefix for the export text
	 * @param functionlist List of functions exported
	 * @param suffix suffic for the export text
	 * @throws Exception
	 */
	public void writeGSkExportsFiltered(FileWriter myWriter, String prefix,List<String> functionlist,String suffix) throws Exception
	{
		int skip = 0;
		for (String name : functionlist) {
			for (String x : ICCencapsulator.GSKUnexports) {
				if (x.equals(name)) {
					skip = 1;
					break;
				}
			}
			if(skip == 0) {
				myWriter.write(prefix + name + suffix);
			}
		}
	}		
	/**
	 * @brief Write a properly formatted exports file for various OS's The exported
	 * functions are the ones currently in the functionnames list This version
	 * writes the exports file GSkit needs.
	 * 
	 * @param myWriter The output stream
	 * @param osnum    The OS type to process.
	 * @param functionlist list of functions to be exported
	 * @param compat Can't remember now :)
	 * @param prefix string prefix
	 */

	public void writeGSKDefFile(FileWriter myWriter, OSTYPE os, List<String> functionlist, String compat, String prefix)
			throws Exception {
		
		switch (os) {
		case WIN:
			myWriter.write("DESCRIPTION 'GSKICCS EXPORT FILE'\n\nEXPORTS\n");
			for (String name : GSKExports) {
				myWriter.write(name + "\n");
			}
			for (String name : GSKWinExports) {
				myWriter.write(name + "\n");
			}
			writeGSkExportsFiltered(myWriter,prefix,functionlist,"\n");
			break;
		case AIX:
			myWriter.write("#!\n*DESCRIPTION 'GSKICCS EXPORT FILE'\n\n");
			for (String name : GSKExports) {
				myWriter.write(name + "\n");
			}
			writeGSkExportsFiltered(myWriter,prefix,functionlist,"\n");
			break;
		case SUN:
		case LINUX:
			myWriter.write("#DESCRIPTION 'GSKICCS EXPORT FILE'\n\n" + compat + " {\n  global:\n");
			for (String name : GSKExports) {
				myWriter.write("    " + name + ";\n");
			}
			writeGSkExportsFiltered(myWriter, "    "+prefix, functionlist,";\n");

			myWriter.write("  local:\n    *;\n};");
			break;
		case HP:
			myWriter.write("#DESCRIPTION 'GSKICCS EXPORT FILE'\n\n");
			for (String name : GSKExports) {
				myWriter.write("+e " + name + "\n");
			}
			writeGSkExportsFiltered(myWriter, "+e "+prefix, functionlist,"\n");

			myWriter.write("+e " + "gskiccs8_loaded_from" + ICCencapsulator.ICC_Version + "\n");
			break;
		case OS2:
			myWriter.write("LIBRARY         icclib  INITINSTANCE\n");
			myWriter.write("DATA NONSHARED\n\n");
			myWriter.write("DESCRIPTION     'GSkit ICC Stub'\n\n");
			myWriter.write("EXPORTS\n");
			for (String name : GSKExports) {
				myWriter.write("\t_" + name + "\n");
			}
			writeGSkExportsFiltered(myWriter, "\t_" + prefix, functionlist,"\n");

			break;
		case OSX:
			for (String name : GSKExports) {
				myWriter.write("_" + name + "\n");
			}
			writeGSkExportsFiltered(myWriter, "_" + prefix, functionlist,"\n");

			break;
		case OS400:
			myWriter.write("STRPGMEXP PGMLVL(*CURRENT) SIGNATURE(\"LIBICCLIB\")\n");
			for (String name : GSKExports) {
				myWriter.write("EXPORT SYMBOL(\"" + name + "\")\n");
			}
			writeGSkExportsFiltered(myWriter, "EXPORT SYMBOL(\"" + prefix, functionlist,"\")\n");

			myWriter.write("ENDPGMEXP\n");
			break;
		case ZOS:
			myWriter.write("/* z/OS pragma's to control symbol visibility */\n\n");
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("extern \"C\" {\n");
			myWriter.write("#endif\n\n");
			for (String name : GSKExports) {
				myWriter.write("#pragma export(" + name + ")\n");
			}
			writeGSkExportsFiltered(myWriter, "#pragma export(" + prefix, functionlist, ")\n");

			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("};\n");
			myWriter.write("#endif\n");
			break;
		}
	}

	/**
	 * Write a properly formatted exports file for various OS's The exported
	 * functions are the ones currently in the functionnames list This version
	 * writes the exports file GSkit needs.
	 * 
	 * @param myWriter The output stream
	 * @param osnum    The OS type to process.
	 */

	public void writeJGSKDefFile(FileWriter myWriter, OSTYPE os, List<String> functionlist, String compat, String prefix) throws Exception {
		switch (os) {
		case WIN:
			myWriter.write("DESCRIPTION 'GSKICCS EXPORT FILE'\n\nEXPORTS\n");
			for (String name : JGSKExports) {
				myWriter.write(name + "\n");
			}
			for (String name : JGSKWinExports) {
				myWriter.write(name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case AIX:
			myWriter.write("#!\n*DESCRIPTION 'GSKICCS EXPORT FILE'\n\n");
			for (String name : JGSKExports) {
				myWriter.write(name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case SUN:
		case LINUX:
			myWriter.write("#DESCRIPTION 'GSKICCS EXPORT FILE'\n\n" + compat + " {\n  global:\n");
			for (String name : JGSKExports) {
				myWriter.write("    " + name + ";\n");
			}
			for (String name : functionlist) {
				myWriter.write("    " + prefix + name + ";\n");
			}

			myWriter.write("  local:\n    *;\n};");
			break;
		case HP:
			myWriter.write("#DESCRIPTION 'GSKICCS EXPORT FILE'\n\n");
			for (String name : JGSKExports) {
				myWriter.write("+e " + name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write("+e " + prefix + name + "\n");
			}
			myWriter.write("+e " + "jgskiccs8_loaded_from" + ICCencapsulator.ICC_Version + "\n");
			break;
		case OS2:
			myWriter.write("LIBRARY         icclib  INITINSTANCE\n");
			myWriter.write("DATA NONSHARED\n\n");
			myWriter.write("DESCRIPTION     'GSkit ICC Stub'\n\n");
			myWriter.write("EXPORTS\n");
			for (String name : JGSKExports) {
				myWriter.write("\t_" + name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write("\t_" + prefix + name + "\n");
			}
			break;
		case OSX:
			for (String name : JGSKExports) {
				myWriter.write("_" + name + "\n");
			}
			for (String name : functionlist) {
				// we need the '_' prepended here
				myWriter.write("_" + prefix + name + "\n");
			}
			break;
		case OS400:
			myWriter.write("STRPGMEXP PGMLVL(*CURRENT) SIGNATURE(\"LIBICCLIB\")\n");
			for (String name : JGSKExports) {
				myWriter.write("EXPORT SYMBOL(\"" + name + "\")\n");
			}
			for (String name : functionlist) {
				myWriter.write("EXPORT SYMBOL(\"" + prefix + name + "\")\n");
			}
			myWriter.write("ENDPGMEXP\n");
			break;
		case ZOS:
			myWriter.write("/* z/OS pragma's to control symbol visibility */\n\n");
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("extern \"C\" {\n");
			myWriter.write("#endif\n\n");
			for (String name : JGSKExports) {
				myWriter.write("#pragma export(" + name + ")\n");
			}
			for (String name : functionlist) {
				myWriter.write("#pragma export(" + prefix + name + ")\n");
			}
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("};\n");
			myWriter.write("#endif\n");
			break;
		}
	}

	/**
	 * Write a properly formatted exports file for various OS's The exported
	 * functions are the ones currently in the functionnames list This version
	 * writes the exports file GSkit needs.
	 * 
	 * @param myWriter The output stream
	 * @param osnum    The OS type to process.
	 */

	public void writeICKCDefFile(FileWriter myWriter, OSTYPE os, List<String> functionlist, String compat, String prefix) throws Exception {
		switch (os) {
		case WIN:
			myWriter.write("DESCRIPTION 'GSKICCS EXPORT FILE'\n\nEXPORTS\n");
			for (String name : ICKCExports) {
				myWriter.write(name + "\n");
			}
			for (String name : ICKCWinExports) {
				myWriter.write(name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case AIX:
			myWriter.write("#!\n*DESCRIPTION 'GSKICCS EXPORT FILE'\n\n");
			for (String name : ICKCExports) {
				myWriter.write(name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case SUN:
		case LINUX:
			myWriter.write("#DESCRIPTION 'GSKICCS EXPORT FILE'\n\n" + compat + " {\n  global:\n");
			for (String name : ICKCExports) {
				myWriter.write("    " + name + ";\n");
			}
			for (String name : functionlist) {
				myWriter.write("    " + prefix + name + ";\n");
			}

			myWriter.write("  local:\n    *;\n};");
			break;
		case HP:
			myWriter.write("#DESCRIPTION 'GSKICCS EXPORT FILE'\n\n");
			for (String name : ICKCExports) {
				myWriter.write("+e " + name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write("+e " + prefix + name + "\n");
			}
			myWriter.write("+e " + "ickciccs8_loaded_from" + ICCencapsulator.ICC_Version + "\n");
			break;
		case OSX:
			for (String name : ICKCExports) {
				myWriter.write("_" + name + "\n");
			}
			for (String name : functionlist) {
				// we need the '_' prepended here
				myWriter.write("_" + prefix + name + "\n");
			}
			break;
		case OS400:
			myWriter.write("STRPGMEXP PGMLVL(*CURRENT) SIGNATURE(\"LIBICCLIB\")\n");
			for (String name : ICKCExports) {
				myWriter.write("EXPORT SYMBOL(\"" + name + "\")\n");
			}
			for (String name : functionlist) {
				myWriter.write("EXPORT SYMBOL(\"" + prefix + name + "\")\n");
			}
			myWriter.write("ENDPGMEXP\n");
			break;
		case ZOS:
			myWriter.write("/* z/OS pragma's to control symbol visibility */\n\n");
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("extern \"C\" {\n");
			myWriter.write("#endif\n\n");
			for (String name : ICKCExports) {
				myWriter.write("#pragma export(" + name + ")\n");
			}
			for (String name : functionlist) {
				myWriter.write("#pragma export(" + prefix + name + ")\n");
			}
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("};\n");
			myWriter.write("#endif\n");
			break;
		}
	}

	/**
	 * Write a properly formatted exports file for the ICC_AUX library on various
	 * OS's The exported functions are the ones currently in the functionnames list
	 * 
	 * @param myWriter The output stream
	 * @param osnum    The OS type to process.
	 */
	static void writeAUXDefFile(FileWriter myWriter, OSTYPE os, String prefix, List<String> functionlist)
			throws Exception {
		switch (os) {
		case WIN:
			myWriter.write("DESCRIPTION 'ICC_AUX EXPORT FILE'\n\nEXPORTS\n");
			for (String name : AUXExports) {
				myWriter.write(prefix + name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case AIX:
			for (String name : AUXExports) {
				myWriter.write(prefix + name + "\n");
			}
			myWriter.write("#!\n*DESCRIPTION 'ICC_AUX EXPORT FILE'\n\n");
			for (String name : functionlist) {
				myWriter.write(prefix + name + "\n");
			}
			break;
		case SUN:
		case LINUX:
			myWriter.write("#DESCRIPTION 'ICC_AUX EXPORT FILE'\n\nOPENSSL {\n  global:\n");
			for (String name : AUXExports) {
				myWriter.write("    " + prefix + name + ";\n");
			}
			for (String name : functionlist) {
				myWriter.write("    " + prefix + name + ";\n");
			}
			myWriter.write("  local:\n    *;\n};");
			break;
		case HP:

			myWriter.write("#DESCRIPTION 'ICC_AUX EXPORT FILE'\n\n");
			for (String name : AUXExports) {
				myWriter.write("+e " + prefix + name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write("+e " + prefix + name + "\n");
			}
			break;
		case OS2:
			myWriter.write("LIBRARY         ICC_AUX  INITINSTANCE\n");
			myWriter.write("DATA NONSHARED\n\n");
			myWriter.write("DESCRIPTION     'ICC Auxiliary Shared Library'\n\n");
			myWriter.write("EXPORTS\n");
			for (String name : AUXExports) {
				myWriter.write("\t_" + prefix + name + "\n");
			}
			for (String name : functionlist) {
				myWriter.write("\t_" + prefix + name + "\n");
			}
			break;
		case OSX:
			for (String name : AUXExports) {
				// we need the '_' prepended here
				myWriter.write("_" + prefix + name + "\n");
			}
			for (String name : functionlist) {
				// we need the '_' prepended here
				myWriter.write("_" + prefix + name + "\n");
			}
			break;
		case OS400:
			myWriter.write("STRPGMEXP PGMLVL(*CURRENT) SIGNATURE(\"LIBICC_AUX\")\n");
			for (String name : AUXExports) {
				// we need the '_' prepended here
				myWriter.write("EXPORT SYMBOL(\"" + prefix + name + "\")\n");
			}
			for (String name : functionlist) {
				// we need the '_' prepended here
				myWriter.write("EXPORT SYMBOL(\"" + prefix + name + "\")\n");
			}
			myWriter.write("ENDPGMEXP\n");
			break;
		case ZOS:
			myWriter.write("/* z/OS pragma's to control symbol visbility */\n\n");
			myWriter.write("#ifdef __cplusplus\n");
			myWriter.write("extern \"C\" {\n");
			myWriter.write("#endif\n\n");
			for (String name : AUXExports) {
				// we need the '_' prepended here
				myWriter.write("#pragma export(" + prefix + name + ")\n");
			}
			for (String name : functionlist) {
				// we need the '_' prepended here
				myWriter.write("#pragma export(" + prefix + name + ")\n");
			}
			myWriter.write("\n#ifdef __cplusplus\n");
			myWriter.write("};\n");
			myWriter.write("#endif\n");
			break;
		}
	}
}
// Reads ICC_ver.txt, pulls out the ICC version string from that (if not version is 0_0_0)
// and makes it avilable to the rest of the code.
// Needed to create extra exported symbols for HPUX PA-RISC 32 bit


class ICCVersion
{
    static String myfile = "ICC_ver.txt";
    static String  vstr = "0_0_0";
    static Boolean init = false;
    ICCVersion() throws Exception {
	Readit();
    }
    static String Readit() {
	try {
	    File f = new File(myfile);
	    if(f.exists()) {
		BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream(myfile)));
		vstr = reader.readLine();
		vstr = vstr.trim();
		/* System.out.println("ICCVersion ["+vstr+"]"); */
	    }
	    init = true;
	}
	catch(IOException e) {

	}
	return vstr;
    }
    static public String GetVersion() {
	if(!init) {
	    vstr = Readit();
	}
	return vstr;
    }

}
