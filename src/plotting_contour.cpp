/***************************************************************************
                       plotting.cpp  -  GDL routines for plotting
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002-2011 by Marc Schellens et al.
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"
#include "plotting.hpp"
#include "math_utl.hpp"

namespace lib {

  using namespace std;

  struct mypltr_passinfo // {{{
  {
    PLFLT spa[4];
#ifdef USE_LIBPROJ4
    PLFLT sx[2], sy[2];
    LPTYPE* idata;
    XYTYPE* odata; 
    PROJTYPE* ref; 
    DDouble d_nan;
    bool mapSet;
#endif
  }; // }}}

  void mypltr(PLFLT x, PLFLT y, PLFLT *tx, PLFLT *ty, void *pltr_data) // {{{
  {
    PLFLT tr[6]={0.0,0.0,0.0,0.0,0.0,0.0};
    struct mypltr_passinfo *ptr = (mypltr_passinfo* )pltr_data;

    tr[0] = ptr->spa[0];
    tr[4] = ptr->spa[1];
    tr[2] = ptr->spa[2];
    //    tr[5] = ptr->spa[4];
    tr[5] = ptr->spa[3];

    // conversion from array indices to data coord
    x = tr[0] * x + tr[2];
    y = tr[4] * y + tr[5];
    
    // conversion from lon / lat to projected values (in normal coordinates)
#ifdef USE_LIBPROJ4
    if (ptr->mapSet)
    {
      // Convert from lon/lat in degrees to radians
      ptr->idata->lam = x * DEG_TO_RAD;
      ptr->idata->phi = y * DEG_TO_RAD;
      
      // Convert from lon/lat in radians to data coord
      *ptr->odata = PJ_FWD(*ptr->idata, ptr->ref);
      x = ptr->odata->x;
      y = ptr->odata->y;

      // handling inf points (not sure if this is needed?)
      if (!isfinite(x) || !isfinite(y)) x = y = ptr->d_nan;
    }
#endif

    // assignment to pointers passed in arguments
    *tx = x;
    *ty = y;
  } // }}}

  void contour( EnvT* e)
  {
    int debug=0;
    
    SizeT nParam=e->NParam( 1); 
    
    DDoubleGDL* zVal;
    DDoubleGDL* yVal;
    DDoubleGDL* xVal;
    //    DDoubleGDL* zValT;
    auto_ptr<BaseGDL> xval_guard;
    auto_ptr<BaseGDL> yval_guard;
    auto_ptr<BaseGDL> p0_guard;

    SizeT xEl;
    SizeT yEl;
    SizeT zEl;
    if( nParam == 1)
      {
	BaseGDL* p0 = e->GetNumericArrayParDefined( 0)->Transpose( NULL);

	zVal = static_cast<DDoubleGDL*>
	  (p0->Convert2( DOUBLE, BaseGDL::COPY));
	p0_guard.reset( p0); // delete upon exit

	xEl = zVal->Dim(1);
	yEl = zVal->Dim(0);

	if(zVal->Rank() != 2)
	  e->Throw(  "Array must have 2 dimensions: "
			      +e->GetParString(0));

	xVal = new DDoubleGDL( dimension( xEl), BaseGDL::INDGEN);
	xval_guard.reset( xVal); // delete upon exit
	yVal = new DDoubleGDL( dimension( yEl), BaseGDL::INDGEN);
	yval_guard.reset( yVal); // delete upon exit
      } else if ( nParam == 2 || nParam > 3) {
	e->Throw( "Incorrect number of arguments.");
      } else {
	BaseGDL* p0 = e->GetNumericArrayParDefined( 0)->Transpose( NULL);
	zVal = static_cast<DDoubleGDL*>
	  (p0->Convert2( DOUBLE, BaseGDL::COPY));
	p0_guard.reset( p0); // delete upon exit

	if(zVal->Dim(0) == 1)
	  e->Throw( "Array must have 2 dimensions: "
			      +e->GetParString(0));

	xVal = e->GetParAs< DDoubleGDL>( 1);
	yVal = e->GetParAs< DDoubleGDL>( 2);

	if (xVal->Rank() > 2)
	  e->Throw( "X, Y, or Z array dimensions are incompatible.");

	if (yVal->Rank() > 2)
	  e->Throw( "X, Y, or Z array dimensions are incompatible.");

	if (xVal->Rank() == 1) {
	  xEl = xVal->Dim(0);

	  if(xEl != zVal->Dim(1))
	    e->Throw( "X, Y, or Z array dimensions are incompatible.");
	}

	if (yVal->Rank() == 1) {
	  yEl = yVal->Dim(0);

	  if(yEl != zVal->Dim(0))
	    e->Throw( "X, Y, or Z array dimensions are incompatible.");
	}

	if (xVal->Rank() == 2) {
	  if((xVal->Dim(0) != zVal->Dim(1)) && (xVal->Dim(1) != zVal->Dim(0)))
	    e->Throw( "X, Y, or Z array dimensions are incompatible.");
	}

	if (yVal->Rank() == 2) {
	  if((yVal->Dim(0) != zVal->Dim(1)) && (yVal->Dim(1) != zVal->Dim(0)))
	    e->Throw( "X, Y, or Z array dimensions are incompatible.");
	}
      }

    // !P 
    DLong p_background; 
    DLong p_noErase; 
    DLong p_color; 
    DLong p_psym; 
    DLong p_linestyle;
    DFloat p_symsize; 
    DFloat p_charsize; 
    DFloat p_thick; 
    DString p_title; 
    DString p_subTitle; 
    DFloat p_ticklen; 
    
    GetPData( p_background,
	      p_noErase, p_color, p_psym, p_linestyle,
	      p_symsize, p_charsize, p_thick,
	      p_title, p_subTitle, p_ticklen);

    // !X, !Y, !Z (also used below)
    static DStructGDL* xStruct = SysVar::X();
    static DStructGDL* yStruct = SysVar::Y();
    static DStructGDL* zStruct = SysVar::Z();
    DLong xStyle; 
    DLong yStyle; 
    DLong zStyle; 
    DString xTitle; 
    DString yTitle; 
    DString zTitle; 
    DFloat x_CharSize; 
    DFloat y_CharSize; 
    DFloat z_CharSize; 
    DFloat xMarginL; 
    DFloat xMarginR; 
    DFloat yMarginB; 
    DFloat yMarginF; 
    DFloat zMarginB; 
    DFloat zMarginT; 
    DFloat xTicklen;
    DFloat yTicklen;
    DFloat zTicklen;
    GetAxisData( xStruct, xStyle, xTitle, x_CharSize, xMarginL, xMarginR,
		 xTicklen);
    GetAxisData( yStruct, yStyle, yTitle, y_CharSize, yMarginB, yMarginF,
		 yTicklen);
    GetAxisData( zStruct, zStyle, zTitle, z_CharSize, zMarginB, zMarginT,
		 zTicklen);

    // [XY]STYLE
    e->AssureLongScalarKWIfPresent( "XSTYLE", xStyle);
    e->AssureLongScalarKWIfPresent( "YSTYLE", yStyle);
    e->AssureLongScalarKWIfPresent( "ZSTYLE", zStyle);

    // TITLE
    DString title = p_title;
    DString subTitle = p_subTitle;
    e->AssureStringScalarKWIfPresent( "TITLE", title);
    e->AssureStringScalarKWIfPresent( "SUBTITLE", subTitle);

    // AXIS TITLE
    e->AssureStringScalarKWIfPresent( "XTITLE", xTitle);
    e->AssureStringScalarKWIfPresent( "YTITLE", yTitle);
    e->AssureStringScalarKWIfPresent( "ZTITLE", zTitle);

    // MARGIN (in characters)
    static int xMarginEnvIx = e->KeywordIx( "XMARGIN"); 
    static int yMarginEnvIx = e->KeywordIx( "YMARGIN"); 
    static int zMarginEnvIx = e->KeywordIx( "ZMARGIN"); 
    BaseGDL* xMargin = e->GetKW( xMarginEnvIx);
    BaseGDL* yMargin = e->GetKW( yMarginEnvIx);
    BaseGDL* zMargin = e->GetKW( zMarginEnvIx);
    if( xMargin != NULL)
      {
	if( xMargin->N_Elements() > 2)
	  e->Throw( "Keyword array parameter XMARGIN"
		    " must have from 1 to 2 elements.");
	auto_ptr<DFloatGDL> guard;
	DFloatGDL* xMarginFl = static_cast<DFloatGDL*>
	  ( xMargin->Convert2( FLOAT, BaseGDL::COPY));
	guard.reset( xMarginFl);
	xMarginL = (*xMarginFl)[0];
	if( xMarginFl->N_Elements() > 1)
	  xMarginR = (*xMarginFl)[1];
      }
    if( yMargin != NULL)
      {
	if( yMargin->N_Elements() > 2)
	  e->Throw( "Keyword array parameter YMARGIN"
		    " must have from 1 to 2 elements.");
	auto_ptr<DFloatGDL> guard;
	DFloatGDL* yMarginFl = static_cast<DFloatGDL*>
	  ( yMargin->Convert2( FLOAT, BaseGDL::COPY));
	guard.reset( yMarginFl);
	yMarginB = (*yMarginFl)[0];
	if( yMarginFl->N_Elements() > 1)
	  yMarginF = (*yMarginFl)[1];
      }
    if( zMargin != NULL)
      {
	if( zMargin->N_Elements() > 2)
	  e->Throw( "Keyword array parameter ZMARGIN"
		    " must have from 1 to 2 elements.");
	auto_ptr<DFloatGDL> guard;
	DFloatGDL* zMarginFl = static_cast<DFloatGDL*>
	  ( zMargin->Convert2( FLOAT, BaseGDL::COPY));
	guard.reset( zMarginFl);
	zMarginB = (*zMarginFl)[0];
	if( zMarginFl->N_Elements() > 1)
	  zMarginT = (*zMarginFl)[1];
      }

    // x and y and z range
    DDouble xStart;// = xVal->min(); 
    DDouble xEnd;//   = xVal->max(); 
    GetMinMaxVal( xVal, &xStart, &xEnd);
    
    DDouble yStart;// = yVal->min(); 
    DDouble yEnd;//   = yVal->max(); 
    GetMinMaxVal( yVal, &yStart, &yEnd);

    DDouble zStart;// = zVal->min(); 
    DDouble zEnd;//   = zVal->max(); 
    GetMinMaxVal( zVal, &zStart, &zEnd);
    
    if ((xStyle & 1) != 1) {
      PLFLT intv;
      intv = AutoIntvAC(xStart, xEnd, false );
    }

    if ((yStyle & 1) != 1) {
      PLFLT intv;
      intv = AutoIntvAC(yStart, yEnd, false );
    }
    
    if ((zStyle & 1) != 1) {
      PLFLT zintv;
      zintv = AutoIntvAC(zStart, zEnd, false );
    }

    //[x|y|z]range keyword
    static int zRangeEnvIx = e->KeywordIx("ZRANGE");
    static int yRangeEnvIx = e->KeywordIx("YRANGE");
    static int xRangeEnvIx = e->KeywordIx("XRANGE");
    BaseGDL* xRange = e->GetKW( xRangeEnvIx);
    BaseGDL* yRange = e->GetKW( yRangeEnvIx);
    BaseGDL* zRange = e->GetKW( zRangeEnvIx);
    
    if(xRange != NULL) 
      {
	if(xRange->N_Elements() != 2)
	  e->Throw("Keyword array parameter XRANGE"
		   " must have 2 elements.");
	auto_ptr<DFloatGDL> guard;
	DFloatGDL* xRangeF = static_cast<DFloatGDL*>
	  ( xRange->Convert2( FLOAT, BaseGDL::COPY));
	guard.reset( xRangeF);
	xStart = (*xRangeF)[0];
	xEnd = (*xRangeF)[1];
      }

    if(yRange != NULL)
      {
	if(yRange->N_Elements() != 2)
	  e->Throw("Keyword array parameter YRANGE"
		   " must have 2 elements.");
	auto_ptr<DFloatGDL> guard;
	DFloatGDL* yRangeF = static_cast<DFloatGDL*>
	  ( yRange->Convert2( FLOAT, BaseGDL::COPY));
	guard.reset( yRangeF);
	yStart = (*yRangeF)[0];
	yEnd = (*yRangeF)[1];
      }
    if(zRange != NULL)
      {
	if(zRange->N_Elements() != 2)
	  e->Throw("Keyword array parameter ZRANGE"
		   " must have 2 elements.");
	auto_ptr<DFloatGDL> guard;
	DFloatGDL* zRangeF = static_cast<DFloatGDL*>
	  ( zRange->Convert2( FLOAT, BaseGDL::COPY));
	guard.reset( zRangeF);
	zStart = (*zRangeF)[0];
	zEnd = (*zRangeF)[1];
      }

    bool mapSet = false;
#ifdef USE_LIBPROJ4
    get_mapset(mapSet);
#endif

    DDouble minVal = zStart;
    DDouble maxVal = zEnd;
    e->AssureDoubleScalarKWIfPresent( "MIN_VALUE", minVal);
    e->AssureDoubleScalarKWIfPresent( "MAX_VALUE", maxVal);

    // AC july 2008 please remember that data sweep out by that
    // are really processed like MISSING data (NaN ...)
    if (minVal > zStart) cout << "This MIN_VALUE is not ready, sorry. Help welcome." <<endl;
    if (maxVal < zEnd) cout << "This MAX_VALUE is not ready, sorry. Help welcome." <<endl;
    //cout << "MIN_VALUE" << minVal << endl;
    //cout << "MAX_VALUE" << maxVal << endl;

    DLong xTicks=0, yTicks=0, zTicks=0;
    e->AssureLongScalarKWIfPresent( "XTICKS", xTicks);
    e->AssureLongScalarKWIfPresent( "YTICKS", yTicks);
    e->AssureLongScalarKWIfPresent( "ZTICKS", zTicks);

    DLong xMinor=0, yMinor=0, zMinor=0; 
    e->AssureLongScalarKWIfPresent( "XMINOR", xMinor);
    e->AssureLongScalarKWIfPresent( "YMINOR", yMinor);
    e->AssureLongScalarKWIfPresent( "ZMINOR", zMinor);

    DString xTickformat, yTickformat, zTickformat;
    e->AssureStringScalarKWIfPresent( "XTICKFORMAT", xTickformat);
    e->AssureStringScalarKWIfPresent( "YTICKFORMAT", yTickformat);
    e->AssureStringScalarKWIfPresent( "ZTICKFORMAT", zTickformat);

    bool xLog = e->KeywordSet( "XLOG");
    bool yLog = e->KeywordSet( "YLOG");
    bool zLog = e->KeywordSet( "ZLOG");
    if( xLog && xStart <= 0.0)
      Warning( "PLOT: Infinite x plot range.");
    if( yLog && yStart <= 0.0)
      Warning( "PLOT: Infinite y plot range.");
    if( zLog && zStart <= 0.0)
      Warning( "PLOT: Infinite z plot range.");

    DDouble ticklen = p_ticklen;
    e->AssureDoubleScalarKWIfPresent( "TICKLEN", ticklen);
						 
    DLong noErase = p_noErase;
    if( e->KeywordSet( "NOERASE")) noErase = 1;
    
    // POSITION
    PLFLT xScale = 1.0;
    PLFLT yScale = 1.0;
    //    PLFLT scale = 1.0;
    static int positionIx = e->KeywordIx( "POSITION"); 
    DFloatGDL* pos = e->IfDefGetKWAs<DFloatGDL>( positionIx);
    if (pos == NULL) pos = (DFloatGDL*) 0xF;
    /*
    PLFLT position[ 4] = { 0.0, 0.0, 1.0, 1.0};
    if( pos != NULL)
      {
      for( SizeT i=0; i<4 && i<pos->N_Elements(); ++i)
	position[ i] = (*pos)[ i];

      xScale = position[2]-position[0];
      yScale = position[3]-position[1];
      //      scale = sqrt( pow( xScale,2) + pow( yScale,2));
      }
    */

    // CHARSIZE
    DDouble charsize = p_charsize;
    e->AssureDoubleScalarKWIfPresent( "CHARSIZE", charsize);
    if( charsize <= 0.0) charsize = 1.0;
    //    charsize *= scale;

    // AXIS CHARSIZE
    DDouble xCharSize = x_CharSize;
    e->AssureDoubleScalarKWIfPresent( "XCHARSIZE", xCharSize);
    if( xCharSize <= 0.0) xCharSize = 1.0;

    DDouble yCharSize = y_CharSize;
    e->AssureDoubleScalarKWIfPresent( "YCHARSIZE", yCharSize);
    if( yCharSize <= 0.0) yCharSize = 1.0;
    //    yCharSize *= scale;

    DDouble zCharSize = z_CharSize;
    e->AssureDoubleScalarKWIfPresent( "ZCHARSIZE", zCharSize);
    if( zCharSize <= 0.0) zCharSize = 1.0;


    // THICK
    DFloat thick = p_thick;
    e->AssureFloatScalarKWIfPresent( "THICK", thick);

   // CHARTHICK (thickness of "char")
    PLINT charthick=1;

    GDLGStream* actStream = GetPlotStream( e); 

    static int overplotKW = e->KeywordIx("OVERPLOT");
    bool overplot = e->KeywordSet( overplotKW);
    
    DDouble *sx, *sy;
    DFloat *wx, *wy;
    GetSFromPlotStructs(&sx, &sy);
    GetWFromPlotStructs(&wx, &wy);

    // mapping only in OVERPLOT mode
    if (!overplot) set_mapset(0);

    if (overplot) 
    {
      //rewrite these quantities
      if (!mapSet) 
      {
        get_axis_crange("X", xStart, xEnd);
        get_axis_crange("Y", yStart, yEnd);
      } 
      else 
      {
        DataCoordLimits(sx, sy, wx, wy, &xStart, &xEnd, &yStart, &yEnd, true);
      }
      get_axis_margin("X",xMarginL, xMarginR);
      get_axis_margin("Y",yMarginB, yMarginF);
      get_axis_type("X", xLog);
      get_axis_type("Y", yLog);
      DFloat charsizeF;
      gkw_charsize(e,actStream, charsizeF, false);
      charsize=charsizeF;
      pos = NULL;
    }

   // *** start drawing
    gkw_background(e, actStream);  //BACKGROUND
    gkw_color(e, actStream);       //COLOR

    if (!overplot) {
      actStream->NextPlot( !noErase);
      if( !noErase) actStream->Clear();
    }

    // plplot stuff
    // set the charsize (scale factor)
    DDouble charScale = 1.0;
    DLongGDL* pMulti = SysVar::GetPMulti();
    if( (*pMulti)[1] > 2 || (*pMulti)[2] > 2) charScale = 0.5;
    actStream->schr( 0.0, charsize * charScale);

#if 0
    // get subpage in mm
    PLFLT scrXL, scrXR, scrYB, scrYF;
    actStream->gspa( scrXL, scrXR, scrYB, scrYF); 
    PLFLT scrX = scrXR-scrXL;
    PLFLT scrY = scrYF-scrYB;
#endif

    // get char size in mm (default, actual)
    PLFLT defH, actH;
    actStream->gchr( defH, actH);

    // CLIPPING
    DDoubleGDL* clippingD=NULL;
    DLong noclip=0;
    e->AssureLongScalarKWIfPresent( "NOCLIP", noclip);
    if(noclip == 0)
      {
	static int clippingix = e->KeywordIx( "CLIP"); 
	clippingD = e->IfDefGetKWAs<DDoubleGDL>( clippingix);
      }
    
    if (!overplot || !mapSet)
    {
      // viewport and world coordinates
      bool okVPWC = SetVP_WC( e, actStream, overplot?NULL:pos, clippingD,
                            xLog, yLog,
                            xMarginL, xMarginR, yMarginB, yMarginF,
                            xStart, xEnd, yStart, yEnd);
      if( !okVPWC) return;
    } else {
      // not using SetVP_WC as it seem to always select full window for plotting (FIXME)
      actStream->NoSub();
      actStream->vpor(wx[0], wx[1], wy[0], wy[1]);
      actStream->wind( xStart, xEnd, yStart, yEnd);
    }

    // managing the levels list OR the nlevels value

    PLINT nlevel;
    PLFLT *clevel;
    ArrayGuard<PLFLT> clevel_guard;

    if (debug == 1) {
      cout << "zStart :" << zStart <<", zEnd :" << zEnd << endl;
    }

    // we need to define the NaN value
    static DStructGDL *Values =  SysVar::Values();       
    DDouble d_nan=(*static_cast<DDoubleGDL*>(Values->GetTag(Values->Desc()->TagIndex("D_NAN"), 0)))[0]; 
    
    static int levelsix = e->KeywordIx( "LEVELS"); 

    BaseGDL* b_levels=e->GetKW(levelsix);
    if(b_levels != NULL) {
      DDoubleGDL* d_levels = e->GetKWAs<DDoubleGDL>( levelsix);
      nlevel = d_levels->N_Elements();
      clevel = (PLFLT *) &(*d_levels)[0];
      // are the levels ordered ?
      for ( SizeT i=1; i<nlevel; i++) {
	if (clevel[i] <= clevel[i-1]) 
	  e->Throw( "Contour levels must be in increasing order.");
      }      
    } else {
      PLFLT zintv;
      // Jo: added keyword NLEVELS
      if (e->KeywordSet( "NLEVELS")) {
        DLong l_nlevel = nlevel; // GCC 3.4.4 needs that
      	e->AssureLongScalarKWIfPresent( "NLEVELS", l_nlevel);
        nlevel = l_nlevel;
	if (nlevel <= 0) nlevel= 2;  // AC: mimication of IDL

        // cokhavim: IDL does this...
        zintv = (PLFLT) ((zEnd - zStart) / (nlevel+1));
        // ... but I think this is better:
        // if (e->KeywordSet( "FILL")) zintv = (PLFLT) ((zEnd - zStart) / (nlevel));
        // else zintv = (PLFLT) ((zEnd - zStart) / (nlevel+1));

        // SA: this indeed seems better in some cases; however, it makes calls
        //     with and without the /FILL keyword behave differently. As a result,
        //     when overlaing contours over a filled contour, the contours do not match, e.g.:
        //     a=dist(7) & contour,a,/fill,nl=5 & contour,a,/over,/foll,nl=5

      } else {
	zintv = AutoTick(zEnd - zStart);
	nlevel = (PLINT) floor((zEnd - zStart) / zintv);
        // SA: sanity check to prevent segfaults, e.g. with solely non-finite values 
        if (zintv == 0 || nlevel < 0) nlevel = 0; 
      }
      if (debug) cout << "internal Nlevels == " << nlevel << endl;




//       clevel = new PLFLT[nlevel+1];
//       clevel_guard.Reset( clevel);
//       // Jo: fixed clevel to account for non-zero zMin
//       for( SizeT i=1; i<=nlevel; i++) clevel[i-1] = zintv * (i-1) + zStart;
//       //for( SizeT i=0; i<=nlevel; i++) clevel[i] = zintv * i + zStart;
DDouble offset=0.;
if (e->KeywordSet( "FILL")) { nlevel = nlevel + 1; offset=zintv;}
clevel = new PLFLT[nlevel];
clevel_guard.Reset( clevel);
//IDL does this:
// for( SizeT i=1; i<=nlevel; i++) clevel[i-1] = zintv * i + zStart;
//but I think this is better:
for( SizeT i=1; i<=nlevel; i++) clevel[i-1] = zintv * i + zStart - offset;
clevel[nlevel-1]=zEnd; //make this explicit

    }
    // AC would like to check the values (nlevel and values) ...
    if (debug == 1) {
      cout << "Nlevels == " << nlevel << endl;
      for (SizeT i=0; i<=nlevel; i++) cout << i << " " << clevel[i] << endl;
    }
    
//     // Jo: added keyword FILL

    PLINT  &nlevel_fill = nlevel;
    PLFLT* &clevel_fill = clevel;

//     PLFLT *clevel_fill;
//     ArrayGuard<PLFLT> clevel_fill_guard;
//     PLINT nlevel_fill;
//     if (e->KeywordSet( "FILL")) {
//       // To ensure that the highest level is filled, define a new
//       // clevel to include highest value of z:   
//       // modif by AC to manage the exception (nlevel=1)
//       if (nlevel > 1) {
// 	nlevel_fill=nlevel+1;
// 	clevel_fill = new PLFLT[nlevel_fill];
// 	clevel_fill_guard.Reset( clevel_fill);
// 	clevel_fill[nlevel_fill-1] = clevel[nlevel - 1] < zEnd ? zEnd : clevel[nlevel - 1] + 1.;
// 	for( SizeT i=0; i<nlevel; i++) clevel_fill[i] = clevel[i];
//       } else {
// 	nlevel_fill=3;
// 	clevel_fill = new PLFLT[nlevel_fill];
// 	clevel_fill_guard.Reset( clevel_fill);
//         clevel_fill[0] = clevel[0] > zStart ? zStart : clevel[0] - 1.;
//         clevel_fill[1] = clevel[0];
//         clevel_fill[2] = clevel[0] < zEnd ? zEnd : clevel[0] + 1.;
//       }
      
//       if (debug ==1 ) {
// 	cout << "zStart "<< zStart << " zEnd "<< zEnd <<endl ;
// 	for( SizeT i=0; i<nlevel_fill; i++) cout << i << " " << clevel_fill[i] << endl;
//       }
//     }

//     // levels outside limits are changed ...
//     for (SizeT i=0; i<=nlevel; i++) {
//       if (clevel[i] < zStart) clevel[i]=zStart;
//       if (clevel[i] > zEnd) clevel[i]=zEnd;
//     }



    // pen thickness for plot
    actStream->wid( static_cast<PLINT>(floor( thick-0.5)));

    // labeling
    bool label = false;
    if (e->KeywordSet("FOLLOW") || e->KeywordSet("C_CHARSIZE")) label = true;
    // TODO: if (e->KeywordSet("C_ANNOTATION") || e->KeywordSet("C_CHARTHICK") || e->KeywordSet("C_LABELS")) label = true;  
    if (e->KeywordSet("FILL")) label = false;
    if (label)
    { 
      // IDL default: 3/4 of the axis charsize (CHARSIZE keyword or !P.CHARSIZE)
      // PlPlot default: .3
      DFloat label_size = .75 * charsize;
      if (e->KeywordSet("C_CHARSIZE")) e->AssureFloatScalarKWIfPresent("C_CHARSIZE", label_size);
      //usage: setcontlabelparam(PLFLT offset, PLFLT size, PLFLT spacing, PLINT active);
      actStream->setcontlabelparam(0.0, (PLFLT)label_size, .3, true);
    }

#ifdef USE_LIBPROJ4
    static LPTYPE idata;
    static XYTYPE odata;
    static PROJTYPE* ref;
    if (mapSet)
    {
      ref = map_init();
      if ( ref == NULL) e->Throw( "Projection initialization failed.");
    }
#endif

    // starting plotting the data
    struct mypltr_passinfo passinfo;

    // 1 DIM X & Y
    if (xVal->Rank() == 1 && yVal->Rank() == 1) {
      PLFLT spa[4];
      
      // don't forgot we have to use the real limits, not the adjusted ones
      DDouble xMin;// = zVal->min(); 
      DDouble xMax;//   = zVal->max(); 
      GetMinMaxVal( xVal, &xMin, &xMax);
      DDouble yMin;// = zVal->min(); 
      DDouble yMax;//   = zVal->max(); 
      GetMinMaxVal( yVal, &yMin, &yMax);
 
      passinfo.spa[0] = (xMax - xMin) / (xEl - 1);
      passinfo.spa[1] = (yMax - yMin) / (yEl - 1);
      passinfo.spa[2] = xMin;
      passinfo.spa[3] = yMin;

#ifdef USE_LIBPROJ4
      passinfo.mapSet = mapSet;
      if (mapSet) // which imposes overplotting
      {
        passinfo.idata = &idata;
        passinfo.odata = &odata;
        passinfo.ref = ref;
        passinfo.d_nan = d_nan;

        passinfo.sx[0] = sx[0];
        passinfo.sx[1] = sx[1];
        passinfo.sy[0] = sy[0];
        passinfo.sy[1] = sy[1];
      }
#endif

      PLFLT** z = new PLFLT*[xEl];
      for( SizeT i=0; i<xEl; i++) z[i] = &(*zVal)[i*yEl];
      
      // plplot knows how to manage NaN but not Infinity ...
      // we remplace Infinity by Nan
      for( SizeT i=0; i<xEl*yEl; i++) {
	if (isinf((*zVal)[i])) (*z)[i]= d_nan;
      }
      // a draft for MaxVal ...
      //      if (maxVal < zEnd) {
      //;	for( SizeT i=0; i<xEl*yEl; i++) {
      //if ((*zVal)[i] > maxVal) (*z)[i]= d_nan;
      //	}
      //}
      
      //      gkw_linestyle(e, actStream);
      //actStream->lsty(2);
      //
      // AC 18 juin 2007 LineStyle and contour
      // NOT READY NOW
      // here we plot the axis 
      // actStream->cont(z, xEl, yEl, 1, xEl, 1, yEl, 
      //		      clevel, 0, mypltr, static_cast<void*>( spa));
      // we recover the linestyle if a !p.linestyle does exist
      //gkw_linestyle_c(e, actStream, TRUE);


      if (e->KeywordSet( "FILL")) {
	// the "clevel_fill, nlevel_fill" have been computed before
        actStream->shades(z, xEl, yEl, NULL, xStart, xEnd, yStart, yEnd,
 			  clevel_fill, nlevel_fill, 2, 0, 0, plstream::fill,
// 			  clevel, nlevel, 2, 0, 0, plstream::fill,
//			  false, mypltr, static_cast<void*>( spa));
                          false, mypltr, static_cast<void*>(&passinfo));
	
	gkw_color(e, actStream);//needs to be called again or else PS files look wrong
	// Redraw the axes just in case the filling overlaps them
	//actStream->box( xOpt.c_str(), xintv, xMinor, "", 0.0, 0);
	//actStream->box( "", 0.0, 0, yOpt.c_str(), yintv, yMinor);
	// pen thickness for axis
	actStream->wid(charthick);
      } else {
        actStream->cont(z, xEl, yEl, 1, xEl, 1, yEl, 
                        clevel, nlevel, mypltr, static_cast<void*>(&passinfo)) ;

      }
      delete[] z;
    }
    
    if (xVal->Rank() == 2 && yVal->Rank() == 2) {
      // FIXME: mapping not supported here yet
      
      PLcGrid2 cgrid2;
      actStream->Alloc2dGrid(&cgrid2.xg,xVal->Dim(0),xVal->Dim(1));
      actStream->Alloc2dGrid(&cgrid2.yg,xVal->Dim(0),xVal->Dim(1));
      cgrid2.nx = xVal->Dim(0);
      cgrid2.ny = xVal->Dim(1);

      for( SizeT i=0; i<xVal->Dim(0); i++) {
	for( SizeT j=0; j<xVal->Dim(1); j++) {
 	  cgrid2.xg[i][j] = (*xVal)[j*(xVal->Dim(0))+i];
	  cgrid2.yg[i][j] = (*yVal)[j*(xVal->Dim(0))+i];
	}
      }

      PLFLT** z = new PLFLT*[xVal->Dim(0)];
      for( SizeT i=0; i<xVal->Dim(0); i++) z[i] = &(*zVal)[i*xVal->Dim(1)];

      for( SizeT i=0; i<xVal->Dim(0)*xVal->Dim(1); i++) {
	if (isinf((*zVal)[i])) (*z)[i]= d_nan;
      }

      if (e->KeywordSet( "FILL")) {
	// the "clevel_fill, nlevel_fill" have been computed before
        actStream->shades(z, xVal->Dim(0), xVal->Dim(1), 
			  NULL, xStart, xEnd, yStart, yEnd,
 			  clevel_fill, nlevel_fill, 2, 0, 0, plstream::fill,
// 			  clevel, nlevel, 2, 0, 0, plstream::fill,
			  false, plstream::tr2, (void *) &cgrid2 );

	gkw_color(e, actStream);//needs to be called again or else PS files look wrong
	// Redraw the axes just in case the filling overlaps them
	//actStream->box( xOpt.c_str(), xintv, xMinor, "", 0.0, 0);
	//actStream->box( "", 0.0, 0, yOpt.c_str(), yintv, yMinor);
	// pen thickness for axis
	actStream->wid(charthick);
      } else {	      
	actStream->cont(z, xVal->Dim(0), xVal->Dim(1), 
			1, xVal->Dim(0), 1, xVal->Dim(1), clevel, nlevel,
			plstream::tr2, (void *) &cgrid2 );
      }
      actStream->Free2dGrid(cgrid2.xg,xVal->Dim(0),xVal->Dim(1));
      actStream->Free2dGrid(cgrid2.yg,xVal->Dim(0),xVal->Dim(1));

      // AC june 07 : symetry for the previous case
      delete[] z;
    }

    //Draw axes after the data because /fill could potentially overlap the axes.
    //... if keyword "OVERPLOT" is not set
    if (!overplot) 
    {
      gkw_background(e, actStream);  //BACKGROUND
      gkw_color(e, actStream);       //COLOR

      // pen thickness for axis
      actStream->wid( 0);

      // axis
      string xOpt = "bcnst";
      string yOpt = "bcnstv";

      DString xTickformat, yTickformat;
      e->AssureStringScalarKWIfPresent( "XTICKFORMAT", xTickformat);
      e->AssureStringScalarKWIfPresent( "YTICKFORMAT", yTickformat);
      AdjustAxisOpts(xOpt, yOpt, xStyle, yStyle, xTicks, yTicks, xTickformat, yTickformat, xLog, yLog);

      // axis titles
      actStream->schr( 0.0, actH/defH * xCharSize);
      actStream->mtex("b",3.5,0.5,0.5,xTitle.c_str());

      // the axis (separate for x and y axis because of charsize)
      PLFLT xintv;
      if (xTicks == 0) {
        xintv = AutoTick(xEnd-xStart);
      } else {
        xintv = (xEnd - xStart) / xTicks;
      }
      //Draw axis if keyword "OVERPLOT" is not set
      actStream->box( xOpt.c_str(), xintv, xMinor, "", 0.0, 0);
      actStream->schr( 0.0, actH/defH * yCharSize);
      actStream->mtex("l",5.0,0.5,0.5,yTitle.c_str());

      // the axis (separate for x and y axis because of charsize)
      PLFLT yintv;
      if (yTicks == 0) {
        yintv = AutoTick(yEnd-yStart);
      } else {
        yintv = (yEnd - yStart) / yTicks;
      }
      actStream->box( "", 0.0, 0, yOpt.c_str(), yintv, yMinor);

      // Get viewpoint parameters and store in WINDOW & S
      PLFLT p_xmin, p_xmax, p_ymin, p_ymax;
      actStream->gvpd (p_xmin, p_xmax, p_ymin, p_ymax);

      DStructGDL* Struct=NULL;
      Struct = SysVar::X();
      static unsigned windowTag = Struct->Desc()->TagIndex( "WINDOW");
      static unsigned sTag = Struct->Desc()->TagIndex( "S");
      if(Struct != NULL) 
      {
        (*static_cast<DFloatGDL*>( Struct->GetTag( windowTag, 0)))[0] = p_xmin;
        (*static_cast<DFloatGDL*>( Struct->GetTag( windowTag, 0)))[1] = p_xmax;
        (*static_cast<DDoubleGDL*>( Struct->GetTag( sTag, 0)))[0] =
          (p_xmin*xEnd - p_xmax*xStart) / (xEnd - xStart);
        (*static_cast<DDoubleGDL*>( Struct->GetTag( sTag, 0)))[1] =
          (p_xmax - p_xmin) / (xEnd - xStart);
      }

      Struct = SysVar::Y();
      if(Struct != NULL) 
      {
        (*static_cast<DFloatGDL*>( Struct->GetTag( windowTag, 0)))[0] = p_ymin;
        (*static_cast<DFloatGDL*>( Struct->GetTag( windowTag, 0)))[1] = p_ymax;

        (*static_cast<DDoubleGDL*>( Struct->GetTag( sTag, 0)))[0] =
          (p_ymin*yEnd - p_ymax*yStart) / (yEnd - yStart);
        (*static_cast<DDoubleGDL*>( Struct->GetTag( sTag, 0)))[1] =
          (p_ymax - p_ymin) / (yEnd - yStart);
      }

      // title and sub title
      actStream->schr( 0.0, 1.25*actH/defH);
      actStream->mtex("t",1.25,0.5,0.5,title.c_str());
      actStream->schr( 0.0, actH/defH); // charsize is reset here
      actStream->mtex("b",5.4,0.5,0.5,subTitle.c_str());
    
    }

    actStream->flush();
    actStream->lsty(1);//reset linestyle

    if (!overplot)
    {
      // set ![XY].CRANGE
      set_axis_crange("X", xStart, xEnd);
      set_axis_crange("Y", yStart, yEnd);

      //set ![x|y].type
      set_axis_type("X",xLog);
      set_axis_type("Y",yLog);
    }
  } // contour		 
  
} // namespace