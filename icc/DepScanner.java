/*************************************************************************
// Copyright IBM Corp. 2023
//
// Licensed under the Apache License 2.0 (the "License").  You may not use
// this file except in compliance with the License.  You can obtain a copy
// in the file LICENSE in the source distribution.
*************************************************************************/

/*************************************************************************
// Description:
//             
// Input:       functions.txt, symbols.txt
// Generates:   stdout list of objects needed for ICC to function
//             
// Function:
// Takes functions.txt which defines the API's exposed by ICC
// and scans the object list generated by "nm *.o" >symbols.txt
//  to produce a makefile stub which
// includes only those objects ICC depends on directly or 
// indirectly to function. i.e. we produce a list of objects
// excluding "dead code"
*************************************************************************/


/* TBD
   exclude files which are linux specific
   any files the scaner may have missed (1 ?!)
   add an include list for each platform (asm etc)
*/

import java.io.*;
// getenv()
import java.lang.System;
import java.util.*;
// exec()
import java.lang.Runtime;

/**
   This reads a file "functions.txt" full of definitions and a directory 
   (full of objects) - works out code dependencies and produces a list
   including only files we depend on as output (in Makefile format)
 
*/

/*
  Class overview

  DepScanner
  reads functions.txt to find the signatures of the functions we use,
  then runs nm over the objects in a 


*/

public class DepScanner
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


    // retained List of functions
    static ArrayList <ICCFunction> funcs;
    // The current "func" class
    static ICCFunction func; 
    // A list of functionames for this file
    static ArrayList <String> functionnames;
    // Prefix we expect OpenSSL symbols to have
    static String OpenSSLPrefix="";
    // Can be modified via functions.txt PREFIX=<text>
    static String Prefix = "";

    static ArrayList<String> nmdata;
    static ArrayList<ICCFile> filelist;

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
    
    /**
     * Parse input read from function.txt, this just keeps slurping data until it has enough
     * @param buf Input buffer
     * @return the position in the input buffer where we stopped.
     */ 
    static int parsefile(char[] buf) throws Exception
    {
        int newstart = 0;
        String temp = new String(buf);
        StringTokenizer tokenizer = new StringTokenizer(temp,";",true);
        while (tokenizer.hasMoreTokens()) {
	    String token = tokenizer.nextToken();
            // System.out.println(token +"\n");
	    
            //okay, now i should have a function
            //or nothing, since the last char should be a ;
            if (tokenizer.hasMoreTokens()) { //then I'm not on my last one, i must have a full function
                //wait, i may have extra whitespace!
                temp = token.trim();
                // System.out.println("token ["+temp+"]");
		if( temp.startsWith("PREFIX=") ) {
		    /* Drop it on the floor */
		} else if (temp.startsWith("OPENSSLPREFIX=") ) {
		    OpenSSLPrefix = temp.substring(14,temp.length());
		} else if( 0 == func.parse(temp)) { // Found a complete function
		    // filetype.Body(func); // emit it's transformed output
		    funcs.add(func); 	 // add it to the list of functions
		    funcnum++;           // increment the function count
		    func = new ICCFunction(); // Create a new function
		} 
	        
		temp = tokenizer.nextToken(); //should be a ;
		// System.out.println("token ["+temp+"]");		
		newstart += token.length() +1;
            } else {
                return newstart;
            }
        }
        //I hit it right on the dot...the last character in the buffer was a ;
        return BUFFERSIZE;
    }
    /** 
     * Read the in-memory list of symbols dumped by NM and 
     * split them out by file
     */

    static void parseSyms( ) 
    {
	String s;
	ICCFile sym;
	filelist = new ArrayList<ICCFile>();
	Iterator<String> item = nmdata.iterator();
	while(item.hasNext()) {
	    sym = new ICCFile();
	    while(item.hasNext()) {
		s = item.next();
		if(0 == sym.parse(s) ) {
		    if(sym.getName().length() > 0 ) {
			filelist.add(sym);
			break; /* Out of this loop and parse the next */
		    }
		}
	    }		
	}
	return;
    }
    /*! @brief dump the clasified symbols from our list of "Files"
      (debug)
     */
    static void dumpSyms( )
    {
	ICCFile sym;
	Iterator<ICCFile> item = filelist.iterator();
	while(item.hasNext()) {
	    sym = item.next();
	    sym.print();
	}
	return;
    }
    /*! @brief dump the list of "Files" that contained referenced symbols
     */
    static void dumpObjs( )
    {
	int i = 0;
	ICCFile sym;
	Iterator<ICCFile> item = filelist.iterator();
	System.out.println("BUILD_OBJS = \\");
	System.out.print("\t");
	while(item.hasNext()) {
	    sym = item.next();
	    if(sym.isDirty()) {
		System.out.print( sym.getName() +" ");
		if((i++ % 8) == 7){
		    System.out.println("\\");
		    System.out.print("\t");
		}
	    }
	}
	System.out.println();
	System.out.println();
        System.out.println("# "+i+" objects");
	return;
    }    
    /*! @brief dump the list of "Files" that contained referenced symbols
     */
    static void dumpAllObjs( )
    {
	ICCFile sym;
	Iterator<ICCFile> item = filelist.iterator();
	while(item.hasNext()) {
	    sym = item.next();
	    System.out.println( "\t" + sym.getName() + " \\" );
       
	}
	return;
    }    
    /*! @brief iterate through all the function names from
         functions.txt, look for them in all the "File", if we find
	 a definition:
	    mark that file as "dirty"
            take all the undef'd symbols from our file and
	      add them to the end of our search list
            delete the found symbol from the search list  
    */
    static void process( ) 
    {
	String t;
	ArrayList<String> symbols = new ArrayList<String>();
	ArrayList<String> newsymbols = new ArrayList<String>();
	functionnames = new ArrayList<String>();
	Iterator<ICCFunction> i0 = funcs.iterator();
	Iterator<String> i1;
	Iterator<ICCFile> i2;


	while( i0.hasNext() ) {
	    t = OpenSSLPrefix + i0.next().getName();
	    // System.out.println(t);
	    symbols.add(t);
	}
	while( symbols.size() > 0 ) {
	    i1 = symbols.iterator();
	    while(i1.hasNext() ) {
		t = i1.next();
		// System.out.print("Looking for "+t+ " ");
		i2 = filelist.iterator();
		while(i2.hasNext() ) {		    
		    ICCFile f = i2.next();
		    // System.out.print(f.getName()+" ");
		    if( f.inMe(t) && !f.isDirty() ) {
			// System.out.print("in "+f.getName()+" ");
			f.setDirty();
			newsymbols.addAll(f.getNeeds());
			// System.out.println("Found "+t+ " in "+f.getName() +
			//		   " Added: "+f.getNeeds().size()) ;
			break;
		    }
		}
		// System.out.println();
	    
	    }
	    symbols.clear();
	    symbols.addAll(newsymbols);
	    newsymbols.clear();
	    // System.out.println("Reprocess: "+symbols.size());
	    // System.out.print(symbols);
	}
    }

    /**
     * @brief Reads one or two functions.txt input files
     * This contains the initialization for the parser
     * and much of the "oh it's that file so I do this" logic.
     * Note that this code originally edited source files - which was bad as
     * any errors tended to clobber existing manual code.
     * We now #include the auto-generated source and headers to avoid that problem.
     * @param inputFile the input file name

     */
    static void doRead(String inputFile)
    {

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
            myReader = new FileReader(inputFile);	    
            int appendpoint = 0;
            isMoreLeft = true;

            char [] buf = new char[BUFFERSIZE];
	    func = new ICCFunction();
	    // Iterate through functions.txt
            while (isMoreLeft) {
                retval = myReader.read(buf,appendpoint,BUFFERSIZE-appendpoint);
                if (retval == -1) {
                    //end of file
                    System.out.println("Reached end of "+inputFile+" unexpectedly...\n");
                    System.exit(1);
                }
                else if (retval < BUFFERSIZE-appendpoint) { // no more left to read
                    isMoreLeft = false;
                }
                int newstart = parsefile(buf);
                // System.out.println(newstart);
                if (newstart == -1) {
                    //error
                    System.exit(-1);
                }
                else if (isMoreLeft) {
                    for (int i = 0; i < BUFFERSIZE-newstart; i++) {
                        buf[i] = buf[newstart+i];
                    }
                    appendpoint = BUFFERSIZE-newstart;
                    for (int i = appendpoint; i < BUFFERSIZE;i++) buf[i] = '\0';
                }
            }

            myReader.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }
    }


    /* Exec didn't work, can't be bothered debugging, so read from a file 
       generated in a wrapping script 
    */
    static void doExec (String fname)
    {
	String line;
	FileReader symfile;
	BufferedReader in;
	try {

	    symfile = new FileReader(fname);
	    in = new BufferedReader(symfile);

	    while(null != ( line = in.readLine())) {
		nmdata.add(line);
	    }
	    in.close();
	}
	catch (Exception e) {
	    e.printStackTrace();
	}
    }

    static void dumpNmData() 
    {
	Iterator itr = nmdata.iterator();
	while(itr.hasNext()) {
	    System.out.println(itr.next());
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
	String tmp = "tmp"; 
	
	
	try {
	    funcs = new ArrayList<ICCFunction>();
	    nmdata = new ArrayList<String>();

	    doRead("functions.txt");  
	    
	    if(args.length > 0) {
		tmp = args[0];
	    }
	    doExec("symbols.txt");
	    // uncomment to confirm we read symbols.txt
	    // dumpNmData();
	    parseSyms();
	    // dumpAllObjs();
	    // uncomment to confirm that we parsed the data
	    // NOTE: Unlike the linker we don't care if it's a
	    // code (T) or data (D) dependency so we tag both as T 
	    // on output
	    // dumpSyms();
	    process();
	    dumpObjs();
	}
	catch (Exception e) {
	    e.printStackTrace();
	}
    }
    


    /**
     * Write the makefile stub containing the core objects
     */
    public void write_mk_stub(List <String> functionlist) throws Exception
    {		

    }

}
// Encapsulates the information we have about an object file
class ICCFile
{
    /* The object file name */
    String myname = "";
    /* The dependencies of this object file "U"  */
    ArrayList<String> needs;
    /* The symbols this object file provides "T" "D" */
    ArrayList<String> has;
    boolean dirty = false;

    public boolean inMe(String target )
    {
	return has.contains(target);
    }

    public String getName() 
    {
	return myname;
    }
    public boolean setDirty()
    {
	dirty = true;
	return dirty;
    }
    public boolean isDirty()
    {
	return dirty;
    }
    public void print()
    {
	System.out.println(myname+":");

	Iterator l = needs.iterator();
	while(l.hasNext()) {
	    System.out.println("\t U "+l.next());
	}
	Iterator p = has.iterator();
	while(p.hasNext()) {
	    System.out.println("\t T "+p.next());
	}
    }

    /*!
      @brief return 1 until no more data 
      @param s the string to parse
    */
    public int parse(String s)
    {
	int rv = 0;
	int l;
	s = s.trim();
	if(s.length() > 3 ) {
	    rv = 1;
	    l = s.indexOf(':');
	    if(l > 0 ) {
		myname = s.substring(0,l);
		needs = new ArrayList<String>();
		has = new ArrayList<String>();
	    } else if ( s.indexOf("_GLOBAL_OFFSET_TABLE_") >= 0) {
		/* Linker fluff - throw it away */	    
	    } else {
		if( (l = s.indexOf("U ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    needs.add(s);
		} 
		if( (l = s.indexOf("T ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		} 
		if( (l = s.indexOf("D ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		}
		if( (l = s.indexOf("R ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		}
		if( (l = s.indexOf("G ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		}
		if( (l = s.indexOf("S ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		}
		if( (l = s.indexOf("V ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		}
		if( (l = s.indexOf("W ")) >= 0 ) {
		    s = s.substring(l + 2,s.length());
		    has.add(s);
		}
	    }
	}
	return rv;
    }

    public ArrayList<String> getNeeds() 
    {
	return needs;
    }
    public ArrayList<String> getHas()
    {
	return has;
    }
}

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
    
    public String comment = ""; // comment text for this function
    public boolean current = false; // instantiated in current functions.txt


    private String modifyerstring = ""; 

    public String getName() 
    {
	return name;
    }

    public void dump()
    {
	System.out.println(returntype + " "+name+"\n"+
			   comment+"\n" );

    }
    public int parse(String s)
    {
        // We save 'doxygen' style comments and add them to generated output.
         if (s.charAt(0) == '#') {

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



	    
	// E flag: We have to process this function on this pass, is it error ensitive ?
	// System.out.println("modifier = "+modifyerstring+"\n");
	  
	if (modifyerstring.indexOf('E') != -1 ) errorsensitive = true;

	// F flag: We need a macro'd pseuodofunction, it's called somewhere with no ICC context
	macrofunction = false;
	if (modifyerstring.indexOf('F') != -1 ) macrofunction = true;

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

}



 




