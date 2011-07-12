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
// Copyright (C) 2011 by Tom Lechner
//


#include "../language.h"
#include "pagerangeinterface.h"
#include "viewwindow.h"
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/transformmath.h>

#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;
#define DBG 


#define PAD 5
#define FUDGE 5.0


// 0:4             4:1           6:1              doc.n-1
// |iv,iii,ii...---|1,2,3...-----|A-1 ... A-10--|
// ^pos
//
// NumStack<double> positions;
// double xscale,yscale;
// flatpoint offset;






//------------------------------------- PageRangeInterface --------------------------------------
	
/*! \class PageRangeInterface 
 * \brief Interface to define and modify page label ranges.
 *
 * This works on PageRange objects kept by Document objects.
 */


#define PART_None          0
#define PART_Label         1
#define PART_DocPageStart  2
#define PART_LabelStart    3
#define PART_Position      4
#define PART_Index         5


PageRangeInterface::PageRangeInterface(int nid,Displayer *ndp,Document *ndoc)
	: InterfaceWithDp(nid,ndp) 
{
	xscale=1;
	yscale=1;

	currange=0;
	hover_part=-1;
	hover_range=-1;
	hover_position=-1;
	hover_index=-1;
	temp_range=NULL;

	showdecs=0;
	firsttime=1;
	doc=NULL;
	UseThisDocument(ndoc);

	defaultbg=rgbcolor(255,255,255);
	defaultfg=rgbcolor(0,0,0);
}

PageRangeInterface::PageRangeInterface(anInterface *nowner,int nid,Displayer *ndp)
	: InterfaceWithDp(nowner,nid,ndp) 
{
	xscale=1;
	yscale=1;

	currange=0;
	hover_part=-1;
	hover_range=-1;
	hover_position=-1;
	hover_index=-1;
	temp_range=NULL;

	showdecs=0;
	firsttime=1;
	doc=NULL;

	defaultbg=rgbcolor(255,255,255);
	defaultfg=rgbcolor(0,0,0);
}

PageRangeInterface::~PageRangeInterface()
{
	DBG cerr <<"PageRangeInterface destructor.."<<endl;

	if (doc) doc->dec_count();
}

const char *PageRangeInterface::Name()
{ return _("Page Range Organizer"); }

//! Return something like "1,2,3,..." or "iv,iii,ii,..."
/*! If range<0, then use currange. If first<0, use default start for that range.
 */
char *PageRangeInterface::LabelPreview(int range,int first,int labeltype)
{
	if (range<0) range=currange;

	char *str=NULL;
	int f,l; //first, length of range
	PageRange *rangeobj=NULL;
	char del=0;

	if (doc->pageranges.n) {
		if (range>=doc->pageranges.n) range=doc->pageranges.n-1;
		rangeobj=doc->pageranges.e[range];

		if (first<0) f=rangeobj->first; 
		else f=first;
		l=rangeobj->end-rangeobj->start+1;
	} else {
		rangeobj=new PageRange(NULL,"#",Numbers_Arabic,0,doc->pages.n-1,1,0);
		del=1;

		if (range>0) range=0;
		l=doc->pages.n;
		f=1;
	}

	if (rangeobj->labeltype==Numbers_None) {
		str=newstr(_("(no numbers)"));
	} else {
		str=rangeobj->GetLabel(rangeobj->start,f,labeltype);
		if (l>1) {
			appendstr(str,",");
			char *strr=rangeobj->GetLabel(rangeobj->start+1,f,labeltype);
			appendstr(str,strr);
			delete[] strr;
			if (l>2) {
				appendstr(str,",");
				char *strr=rangeobj->GetLabel(rangeobj->start+2,f,labeltype);
				appendstr(str,strr);
				delete[] strr;
			}
			appendstr(str,",...");
		}
	}

	if (del) delete rangeobj;
	return str;
}

#define RANGE_Custom_Base   10000
#define RANGE_Reverse       10001
#define RANGE_Delete        10002

/*! \todo much of this here will change in future versions as more of the possible
 *    boxes are implemented.
 */
Laxkit::MenuInfo *PageRangeInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo(_("Paper Interface"));

	menu->AddItem(_("Custom base..."),RANGE_Custom_Base);
	menu->AddItem(_("Reverse"),RANGE_Reverse);
	menu->AddSep();
	if (positions.n>2) {
		menu->AddItem(_("Delete range"),RANGE_Delete);
		menu->AddSep();
	}

	char *str=NULL;
	for (int c=0; c<Numbers_MAX; c++) {
		//if (c==Numbers_Default) menu->AddItem(_("(default)"),c);
		if (c==Numbers_Default) ;
		else if (c==Numbers_None) menu->AddItem(_("(none)"),c);
		else {
			 //add first, first++, first+++, ...  in number format
			str=LabelPreview(currange,-1,c);
			menu->AddItem(str,c);
			delete[] str;
		}
	}

	return menu;
}

/*! Return 0 for menu item processed, 1 for nothing done.
 */
int PageRangeInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i=s->info2; //id of menu item
		if (i==RANGE_Custom_Base) {
			return 0;

		} else if (i==RANGE_Delete) {
			return 0;

		} else if (i==RANGE_Reverse) {
			if (!doc || currange<0) return 0;
			PageRange *r=NULL;
			if (doc->pageranges.n) r=doc->pageranges.e[0];
			if (!r) doc->ApplyPageRange(NULL,Numbers_Arabic,"#",0,-1,doc->pages.n,1);
			else {
				if (r->decreasing) 
					doc->ApplyPageRange(r->name,r->labeltype,r->labelbase,r->start,r->end,r->first-(r->end-r->start),0);
				else
					doc->ApplyPageRange(r->name,r->labeltype,r->labelbase,r->start,r->end,r->first+(r->end-r->start),1);
			}
			needtodraw=1;
			return 0;
		}

		return 0;
	}
	return 1;
}

/*! incs count of ndoc if ndoc is not already the current document.
 *
 * Return 0 for success, nonzero for fail.
 */
int PageRangeInterface::UseThisDocument(Document *ndoc)
{
	if (ndoc==doc) return 0;
	if (doc) doc->dec_count();
	doc=ndoc;
	if (ndoc) ndoc->inc_count();

	MapPositions();

	return 0;
}

//! When a document changes, or its ranges change, we need to remap the positions array.
void PageRangeInterface::MapPositions()
{
	positions.flush();
	if (doc) {
		double total=doc->pageranges.n;
		if (doc->pageranges.n==0) {
			total=1;
			positions.push(0);
		} else {
			for (int c=0; c<doc->pageranges.n; c++) {
				positions.push(doc->pageranges.e[c]->start/((double)doc->pages.n));
				DBG cerr <<"position "<<positions.e[c]<<endl;
			}
		}
		positions.push(1);
	}
	needtodraw=1;
}

//! Use a Document.
int PageRangeInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask)
{
	Document *d=dynamic_cast<Document *>(ndata);
	if (!d && ndata) return 0; //was a non-null object, but not a document

	UseThisDocument(d);
	needtodraw=1;

	return 1;
}

/*! Will say it cannot draw anything.
 */
int PageRangeInterface::draws(const char *atype)
{ return 0; }


//! Return a new PageRangeInterface if dup=NULL, or anInterface::duplicate(dup) otherwise.
/*! 
 */
anInterface *PageRangeInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new PageRangeInterface(id,NULL);
	else if (!dynamic_cast<PageRangeInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}


int PageRangeInterface::InterfaceOn()
{
	DBG cerr <<"pagerangeinterfaceOn()"<<endl;

	yscale=50;
	xscale=dp->Maxx-dp->Minx-2*PAD;
	//if (!doc) UseThisDocument(doc);

	showdecs=2;
	needtodraw=1;
	return 0;
}

int PageRangeInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void PageRangeInterface::Clear(SomeData *d)
{
	yscale=50;
	xscale=dp->Maxx-dp->Minx-2*PAD;
	offset=-flatpoint(dp->Minx+PAD,dp->Maxy-PAD);
}

/*! Draws maybebox if any, then DrawGroup() with the current papergroup.
 */
int PageRangeInterface::Refresh()
{
	if (!needtodraw) return 0;
	needtodraw=0;

	if (firsttime) {
		firsttime=0;
		yscale=50;
		xscale=dp->Maxx-dp->Minx-2*PAD;
		offset=-flatpoint(dp->Minx+PAD,dp->Maxy-PAD);
	}

	DBG cerr <<"PageRangeInterface::Refresh()..."<<positions.n<<endl;

	dp->DrawScreen();

	double w,h=yscale;
	flatpoint o;
	o-=offset;
	o.y-=h;

	 //draw blocks
	char *str=NULL;
	char sstr[30];
	int s,f, th=dp->textheight();
	for (int c=0; c<positions.n-1; c++) {
		w=xscale*(positions.e[c+1]-positions.e[c]);

		DBG cerr<<"PageRange interface drawing rect "<<o.x<<','<<o.y<<" "<<w<<"x"<<h<<"  offset:"<<offset.x<<','<<offset.y<<endl;

		 //draw background color
		if (doc->pageranges.n) dp->NewFG(&doc->pageranges.e[c]->color);
		else dp->NewFG(defaultbg);
		dp->drawrectangle(o.x,o.y, w,h, 1);
		
		 //draw hover indicator for background
		if (c==hover_range && (hover_part==PART_LabelStart || hover_part==PART_DocPageStart || hover_part==PART_Label)) {
			dp->NewFG(coloravg(defaultfg, defaultbg, .9));

			if (hover_part==PART_LabelStart) {
				dp->drawrectangle(o.x+w/2,o.y, w/2,h/2, 1);
			} else if (hover_part==PART_DocPageStart) {
				dp->drawrectangle(o.x,o.y, w/2,h/2, 1);
			} else if (hover_part==PART_Label) {
				dp->drawrectangle(o.x,o.y+h/2, w,h/2, 1);
			}
		}

		 //draw black outline around area
		dp->NewFG((unsigned long)0);
		dp->drawrectangle(o.x,o.y, w,h, 0);

		str=LabelPreview(c,-1,Numbers_Default);
		dp->textout(o.x,o.y+h*3/4, str,-1, LAX_LEFT|LAX_VCENTER);
		delete[] str;

		if (doc->pageranges.n) { s=doc->pageranges.e[c]->start; f=doc->pageranges.e[c]->first; }
		else { s=0; f=1; }

		sprintf(sstr,"%d",s);
		dp->textout(o.x+w/2-th/2,o.y+h/4, sstr,-1, LAX_RIGHT|LAX_VCENTER);
		sprintf(sstr,":");
		dp->textout(o.x+w/2,o.y+h/4, sstr,-1, LAX_HCENTER|LAX_VCENTER);
		sprintf(sstr,"%d",f);
		dp->textout(o.x+w/2+th/2,o.y+h/4, sstr,-1, LAX_LEFT|LAX_VCENTER);

		o.x+=w;
	}
	
	if (hover_part==PART_Position) {
		dp->NewFG(0.,1.,0.);
		o.x=-offset.x + xscale*positions.e[hover_position];
		o.y=-offset.y-h;
		dp->drawrectangle(o.x-FUDGE/2,o.y, FUDGE,h, 1);
	}

	if (hover_part==PART_Index) {
		dp->NewFG(1.,0.,0.);
		o.x=-offset.x + xscale*((double)hover_index/doc->pages.n);
		o.y=-offset.y-h;
		dp->drawrectangle(o.x-FUDGE/2,o.y, FUDGE,h, 1);
	}


	dp->DrawReal();

	return 1;
}


//! Return which position (if over a dividing line) and range (the page range chunk) mouse is over, or -1 for none.
/*! Also returns which part of the range the mouse is over.
 * part is 1 for label preview, 2 for doc page index start, 3 for label start index.
 *
 * If index!=NULL, then return also the document page index that the position corresponds to.
 */
int PageRangeInterface::scan(int x,int y, int *range, int *part, int *index)
{
	if (!doc) return -1;

	flatpoint fp(x,y);
	fp+=offset;
	fp.x/=xscale; 
	fp.y/=-yscale; //now fp in range 0..1
	double fudge=FUDGE/xscale;
	DBG cerr <<"x,y="<<x<<","<<y<<"  pos="<<fp.x<<","<<fp.y<<"  offset="<<offset.x<<","<<offset.y<<endl;

	if (fp.y<0 || fp.y>1) return -1;

	int r=-1, pos=-1;
	for (int c=0; c<positions.n; c++) {
		if (c<positions.n-1 && fp.x>=positions.e[c] && fp.x<=positions.e[c+1]) r=c;
		if (fp.x>=positions.e[c]-fudge && fp.x<=positions.e[c]+fudge) pos=c;
	}

	if (r>=0 && part) {
		if (fp.y<.5) *part=PART_Label;
		else if (fp.x<(positions.e[r]+positions.e[r+1])/2) *part=PART_DocPageStart;
		else *part=PART_LabelStart;
	}

	if (index) { 
		*index=fp.x*doc->pages.n;
		if (*index>=doc->pages.n) *index=-1;
	}

	if (range) *range=r;
	if (part && pos>=0) *part=PART_Position;
	return pos;
}

int PageRangeInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (!doc) return 1;
	if (buttondown.isdown(0,LEFTBUTTON)) return 1; //only let one mouse work
	buttondown.down(d->id,LEFTBUTTON,x,y,state);

	int range=-1, index=-1, part=0;
	int pos=scan(x,y, &range, &part, &index);


	if (part==PART_Position) {
		if (pos>=0) {
		}
		return 0;
	}

	//if (count==2) { // ***
	//}

	return 0;
}

int PageRangeInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!doc) return 1;
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	int range=-1, index=-1, part=0;
	int pos=scan(x,y, &range, &part, &index);

	if (!dragged) {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (!doc->pageranges.n) InstallDefaultRange();
			if (range>=0 && range<doc->pageranges.n && index>doc->pageranges.e[range]->start && index<doc->pageranges.e[range]->end) {
				 //Split range if clicking on an index in a range, but not on range midpoints
				char *name=NULL;
				int f;
				PageRange *r=doc->pageranges.e[range];
				if (r->decreasing) f=r->first-(index-r->start); else f=r->first+(index-r->start);
				if (r->name) { name=newstr(r->name); appendstr(name," (split)"); }
				doc->ApplyPageRange(name,r->labeltype,r->labelbase,index,r->end,f,r->decreasing);
				if (name) delete[] name;

				MapPositions();
				needtodraw=1;
			}
			return 0;
		}

		if ((state&LAX_STATE_MASK)==ControlMask) {
			// *** edit base
		} else {
			// *** edit first
		}
	}

	return 0;
}

//! If the document has no defined page ranges, install a default one.
/*! Return 0 for success or 1 for error.
 */
int PageRangeInterface::InstallDefaultRange()
{
	if (!doc) return 1;
	doc->ApplyPageRange(NULL,Numbers_Arabic,"#",0,-1,1,0);
	return 0;
}

int PageRangeInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (hover_part==PART_Label) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) {
			doc->pageranges.e[currange]->labeltype++;
			if (doc->pageranges.e[currange]->labeltype>=Numbers_MAX)
				doc->pageranges.e[currange]->labeltype=Numbers_None;
		}
		needtodraw=1;
		return 0;
	}

	if (currange>=0 && hover_part==PART_LabelStart) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) doc->pageranges.e[currange]->first++;
		needtodraw=1;
		return 0;
	}

	return 1;
}

int PageRangeInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (hover_part==PART_Label) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) {
			if (doc->pageranges.e[currange]->labeltype==Numbers_Default)
				doc->pageranges.e[currange]->labeltype=Numbers_Arabic;
			else doc->pageranges.e[currange]->labeltype--;
			if (doc->pageranges.e[currange]->labeltype==Numbers_Default)
				doc->pageranges.e[currange]->labeltype=Numbers_MAX-1;
		}
		needtodraw=1;
		return 0;
	}

	if (currange>=0 && hover_part==PART_LabelStart) {
		if (!doc->pageranges.n) InstallDefaultRange();
		currange=hover_range;
		if (currange>=0 && currange<doc->pageranges.n) doc->pageranges.e[currange]->first--;
		needtodraw=1;
		return 0;
	}

	return 1;
}


int PageRangeInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	if (!doc) return 0;

	int range=-1, index=-1, part=0;
	int over=scan(x,y,&range,&part, &index);

	DBG cerr <<"over pos:"<<over<<"  range: "<<range<<"  part:"<<part<<"  index:"<<index<<endl;

	int lx,ly;

	if (!buttondown.any()) {
		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (index<=0) part=PART_None; else part=PART_Index;

			if (part==PART_Index && hover_index!=index) {
				hover_part=part;
				hover_index=index;
				needtodraw=1;
			}
		}
		if (hover_part!=part || hover_range!=range || hover_position!=over) {
			hover_part=part;
			hover_range=range;
			hover_position=over;
			hover_index=index;
			needtodraw=1;
		}

		return 0;
	}

	buttondown.move(mouse->id,x,y, &lx,&ly);
	DBG cerr <<"pr last m:"<<lx<<','<<ly<<endl;

	if ((state&LAX_STATE_MASK)==0) {
		offset.x-=x-lx;
		offset.y-=y-ly;
		needtodraw=1;
		return 0;
	}

	 //^ scales
	if ((state&LAX_STATE_MASK)==ControlMask) {
	}

	 //+^ rotates
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
	}

	return 0;
}

/*!
 * 'a'          select all, or if some are selected, deselect all
 * del or bksp  delete currently selected papers
 *
 * \todo auto tile spread contents
 * \todo revert to other group
 * \todo edit another group
 */
int PageRangeInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr<<" got ch:"<<ch<<"  "<<(state&LAX_STATE_MASK)<<endl;

	if (ch==LAX_Esc) {

	} else if (ch==LAX_Shift) {
	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='a' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='r' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='d' && (state&LAX_STATE_MASK)==0) {
	} else if (ch=='9' && (state&LAX_STATE_MASK)==0) {

	} else if (ch==' ' && (state&LAX_STATE_MASK)==0) {
		yscale=50;
		xscale=dp->Maxx-dp->Minx-2*PAD;
		offset=-flatpoint(dp->Minx+PAD,dp->Maxy-PAD);
		needtodraw=1;
		return 0;
	}

	return 1;
}

int PageRangeInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	return 1;
}


//} // namespace Laidout
