//
// $Id$
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2004-2007 by Tom Lechner
//

#include "project.h"
#include "version.h"

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

//---------------------------- Project ---------------------------------------
/*! \class Project
 * \brief Class holding several documents, as well as various other settings.
 *
 * When laidout is opened and a new document is started, everything goes into
 * the default Project. The project maintains the scratchboard, any project notes,
 * the documents of the project, default directories, and other little tidbits
 * the user might care to associate with the project.
 *
 * Also, the Project maintains its own StyleManager***??????
 *
 * \todo *** implement filelist
 */


//! Constructor, just set name=filename=NULL.
Project::Project()
{ 
	name=filename=NULL;
}

/*! Flush docs (done manually to catch for debugging purposes).
 */
Project::~Project()
{
	docs.flush();
	if (name) delete[] name;
	if (filename) delete[] filename;
}

/*! If doc->saveas in not NULL ***make sure this works right!!!
 * then assume that doc is saved in its own file. Else dump_out
 * the doc.
 *
 * If what==-1, then dump out a pseudocode mockup of the file format.
 */
void Project::dump_out(FILE *f,int indent,int what)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s# Documents can be included in the project file itself, or\n",spc);
		fprintf(f,"%s# merely referenced, which is usually the more convenient way.\n",spc);
		fprintf(f,"%s# If merely referenced, the line will look like:\n\n",spc);
		fprintf(f,"%sDocument blah.doc\n\n",spc);
		fprintf(f,"%s#and the file, if relative pathname is given, is relative to the project file itself.\n",spc);
		fprintf(f,"%s#Moving on, there can be any number of Documents, in the following format:\n\n",spc);
		fprintf(f,"%sDocument\n",spc);
		if (docs.n) docs.e[0]->dump_out(f,indent+2,-1);
		else {
			Document d;
			d.dump_out(f,indent+2,-1);
		}
		return;
	}
	if (docs.n) {
		for (int c=0; c<docs.n; c++) {
			fprintf(f,"%sDocument",spc);
			if (docs.e[c]->saveas) fprintf(f," %s\n",docs.e[c]->saveas);
			else {
				fprintf(f,"\n");
				docs.e[c]->dump_out(f,indent+2,0);
			}
		}
	}
	//*** dump_out the window configs..
}

/*! \todo *** imp me!
 */
void Project::dump_in_atts(LaxFiles::Attribute *att,int flag)
{
	cout <<"*** implement dump_in_atts(LaxFiles::Attribute *att,int flag)!!!"<<endl;
}

/*! Returns 0 for success or nonzero error.
 *
 * \todo ***imp me!
 */
int Project::Load(const char *file)
{
	cout <<"*** implement Project::Load(const char *file)!!!"<<endl;
	return 1;
}

/*! Returns 0 for success or nonzero error.
 */
int Project::Save()
{
	if (!filename || !strcmp(filename,"")) {
		DBG cout <<"**** cannot save, filename is null."<<endl;
		return 2;
	}
	FILE *f=NULL;
	f=fopen(filename,"w");
	if (!f) {
		DBG cout <<"**** cannot save project, file \""<<filename<<"\" cannot be opened for writing."<<endl;
		return 3;
	}

	DBG cout <<"....Saving project to "<<filename<<endl;
//	f=stdout;//***
	fprintf(f,"#Laidout %s Project\n",LAIDOUT_VERSION);
	dump_out(f,0,0);
	
	fclose(f);
	return 0;
}

