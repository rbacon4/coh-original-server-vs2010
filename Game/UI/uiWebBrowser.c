#include "uiWebBrowser.h"
#include "..\libs\HeroBrowser\HeroBrowser.h"

#include "uiGame.h"				//	for start_menu
#include "uiUtilMenu.h"			//	for drawBackground
#include "uiUtil.h"				//	for colors/others
#include "uiNet.h"				//  for NewFeature id
#include "MessageStoreUtil.h"	//	for textStd
#include "cmdgame.h"
#include "player.h"
#include "entity.h"
#include "textureatlas.h"
#include "uiUtilMenu.h"
#include "uiUtilGame.h"
#include "sprite_base.h"
#include "sprite_text.h"
#include "sprite_font.h"
#include "uiDialog.h"
#include "smf_util.h"
#include "rt_queue.h"
#include "input.h"
#include "uiInput.h"
#include "win_init.h"
#include "uiCursor.h"
#include "uiPlaySpanStoreLauncher.h"
#include "tex.h"
#include "tex_gen.h"
#include "ogl.h"		// for GL_RGB
#include "edit_cmd.h"	// for editMode()
#include "uiFocus.h"
#include "language/AppLocale.h"
#include "uiWebUtil.h"
#include "AppRegCache.h"
#include "uiNewFeatures.h"

static bool	g_menu_initialized;
static unsigned int g_saved_fpu_control;	// saved floating point control word

static int g_tex_width = 512;				// texture dimensions and web page dimensions
static int g_tex_height = 512;

static int g_tex_view_top = 0;				// portion of the texture we render
static int g_tex_view_left = 0;
static int g_tex_view_bottom = 512;
static int g_tex_view_right = 512;

static void* g_pixels;
static BasicTexture* g_web_tex_bind;

static int	g_load_progress;	// percent of page loaded
static bool g_repaintRequested; // page needs to be redrawn for the user
static bool g_is_https;			// is this an ssl page?
static bool g_is_link_hovered;
static char g_home_url[1024];	// home url if url is supplied with the 'web' command to enter browser
static bool g_exit_signal;		// signals that user wants to exit the browser

static EHeroWebCursor	g_web_cursor_shape = kWebCursor_Arrow;

// Variables that control the size and placement of the browser

// N.B. CoH UI is currently authored upon a base screen size of 1024x768
// with UI screens applying the necessary scale factors for the real rendering
// resolution.
// Origin is at top left of panel when specifying ui elements
// as well as for mouse and browser coords.

// Note that it is not necessary that the browser render surface is the same size
// as the display panel but that is implied by some recent changes
static int g_panel_top = 0;
static int g_panel_left = 0;
static int g_ui_Top = 0;
static int g_ui_Left = 0;
static int g_ui_Bottom = 0;
static int g_ui_Right = 0;
static float g_panel_zlayer = 0.0f;		// depth of layer

static int g_mouse_in_web_window = 0;

static void* g_browser_focus_token;	// token for participating in focus requests

// in case we need to deny focus change requests or do any special tracking/cleanup
static int uiFocusChangeRequestCallback(void* focusOwner, void* focusRequester)
{
	return 1;	// allow all focus change requests
}

static int convert_virtual_key_to_web_key( int key_val )
{
	int key_web = -1;	// unhandled key

	switch (key_val)
	{
	case INP_F1:			key_web = kWebKey_F1;			break;
	case INP_F2:			key_web = kWebKey_F2;			break;
	case INP_F3:			key_web = kWebKey_F3;			break;
	case INP_F4:			key_web = kWebKey_F4;			break;
	case INP_F5:			key_web = kWebKey_F5;			break;
	case INP_F6:			key_web = kWebKey_F6;			break;
	case INP_F7:			key_web = kWebKey_F7;			break;
	case INP_F8:			key_web = kWebKey_F8;			break;
	case INP_F9:			key_web = kWebKey_F9;			break;
	case INP_F10:			key_web = kWebKey_F10;			break;
	case INP_F11:			key_web = kWebKey_F11;			break;
	case INP_F12:			key_web = kWebKey_F12;			break;

	case INP_LEFT:			key_web = kWebKey_Left;			break;
	case INP_UP:			key_web = kWebKey_Up;			break;
	case INP_RIGHT:			key_web = kWebKey_Right;		break;
	case INP_DOWN:			key_web = kWebKey_Down;			break;
	case INP_PRIOR:			key_web = kWebKey_PageUp;		break;
	case INP_NEXT:			key_web = kWebKey_PageDown;		break;
	case INP_HOME:			key_web = kWebKey_Home;			break;
	case INP_END:			key_web = kWebKey_End;			break;
	case INP_INSERT:		key_web = kWebKey_Insert;		break;
	case INP_DELETE:		key_web = kWebKey_Delete;		break;
	case INP_TAB:			key_web = kWebKey_Tab;			break;
	case INP_RETURN:		key_web = kWebKey_Enter;		break;
	case INP_NUMPADENTER:	key_web = kWebKey_NumPadEnter;	break;
	case INP_ESCAPE:		key_web = kWebKey_Escape;		break;
	case INP_BACK:			key_web = kWebKey_Backspace;	break;
	}

	return key_web;
}

const char* getLocalePlayNC( void )
{
	// special case UK English, otherwise we
	// can use the table lookup used for PlaySpan
	// language string
	if (locIsBritish(getCurrentLocale()))
	{
		return "en-gb";
	}
	else
		return locGetAlpha2( getCurrentLocale() );
}

static const char* get_home_url(void)
{	
	static const char url1[] =	"www.cityofheroes.com";
	static const char url2[] =	"file:///C:/home.html";	// this page can be edited locally to provide a suite of tests and test urls

	const char* home_url;

	// prefer url set when browser was entered as home
	// has to be at least 2 characters
	if (g_home_url[0] && g_home_url[1])
	{
		home_url = g_home_url;
	}
	else
	{
		// otherwise use a default home url
		home_url = GetAsyncKeyState(VK_SHIFT) ? url2 : url1;
	}
	if (cmdAccessLevel()>ACCESS_USER) printf("home: %s", home_url);
	return home_url;
}

bool uiWebBrowser_goto_url(const char* url)
{
	if (!url || !*url)
	{
		if (cmdAccessLevel()>ACCESS_USER) printf("browser: *warning* - goto requested on a blank url!\n");
		return false;
	}
	else
	{
		if (cmdAccessLevel()>ACCESS_USER) printf("browser: goto '%s'\n", url );
		return webBrowser_goto(url);
	}
}

void uiWebBrowser_go_back()
{
	webBrowser_triggerAction(kWebAction_Back);
}

static void convert_ui_to_browser_coords( int* ix, int* iy )
{
	int x, y, nx, ny;
	int ui_origin_x, ui_origin_y;
	int ui_width, ui_height;

	float screenScaleX, screenScaleY;
	calc_scrn_scaling(&screenScaleX, &screenScaleY);

	// calculate browser ui panel location data
	// based on current display resolution
	// @todo no longer correct for an arbitrary g_tex_view_bottom, right
	ui_origin_x = g_panel_left*screenScaleX;
	ui_origin_y = g_panel_top*screenScaleY;
	ui_width = g_tex_width*screenScaleX;
	ui_height = g_tex_height*screenScaleY;

	x = *ix;
	y = *iy;
	nx = (x-ui_origin_x)*g_tex_width/(float)ui_width;
	ny = (y-ui_origin_y)*g_tex_height/(float)ui_height;

	// return to caller
	*ix = nx + g_tex_view_left;
	*iy = ny + g_tex_view_top;
}

static void convert_browser_to_ui_coords( int* ix, int* iy )
{
	int x, y, nx, ny;
	int ui_origin_x, ui_origin_y;
	int ui_width, ui_height;

	float screenScaleX, screenScaleY;
	calc_scrn_scaling(&screenScaleX, &screenScaleY);
	
	// calculate browser ui panel location data
	// based on current display resolution
	// @todo no longer correct for an arbitrary g_tex_view_bottom, right
	ui_origin_x = g_panel_left*screenScaleX;
	ui_origin_y = g_panel_top*screenScaleY;
	ui_width = g_tex_width*screenScaleX;
	ui_height = g_tex_height*screenScaleY;

	x = *ix - g_tex_view_left;
	y = *iy - g_tex_view_top;
	nx = x*ui_width/(float)g_tex_width + ui_origin_x;
	ny = y*ui_height/(float)g_tex_height + ui_origin_y;

	*ix = nx;
	*iy = ny;
}

void uiWebBrowser_update_screen_position(int x, int y, float z_layer)
{
	// update panel location and layer
	g_panel_left = x;
	g_panel_top = y;
	g_panel_zlayer = z_layer;
}

// used to request input focus for the browser, we call this
// when the browser is first opened or when someone clicks in the
// browser window
static bool request_input_focus(void)
{
	int ok_to_focus = uiGetFocusEx(&g_browser_focus_token, TRUE, uiFocusChangeRequestCallback);
	if (ok_to_focus)
	{
		webBrowser_activate();
	}
	return ok_to_focus;
}

static void release_input_focus(void)
{
	uiReturnFocus(&g_browser_focus_token);	// release focus if we have it
	webBrowser_deactivate();
}

bool uiWebBrowser_has_focus(void)
{
	return uiInFocus(&g_browser_focus_token);
}

static void generate_browser_key_event(KeyInput* input)
{
	int modifiers = 0;
	int key_val = input->key;
	int key_type = kWebKeyType_Normal;

	// convert modifier flags for browser abstract
	if (input->attrib&KIA_SHIFT)
		modifiers |= kWebKeyModifier_SHIFT;
	if (input->attrib&KIA_CONTROL)
		modifiers |= kWebKeyModifier_CONTROL;
	if (input->attrib&KIA_ALT)
		modifiers |= kWebKeyModifier_ALT;

	// convert special key codes if necessary
	if (input->type == KIT_EditKey)
	{
		key_val = convert_virtual_key_to_web_key(input->key);
		key_type = kWebKeyType_Special;
	}
	webBrowser_keyEvent( kWebEvent_KeyPress, key_type, key_val, modifiers);
}

// returns true if we converted the input to a web action
static bool generate_browser_key_action(KeyInput* input)
{
	if (input->type == KIT_EditKey)
	{
		if (input->attrib&KIA_CONTROL)
		{
			if (input->key == INP_C)		// CTRL-C
			{
				webBrowser_triggerAction(kWebAction_Copy);
				return true;
			}
			else if (input->key == INP_V)	// CTRL-V
			{
				webBrowser_triggerAction(kWebAction_Paste);
				return true;
			}
			else if (input->key == INP_X)	// CTRL-X
			{
				webBrowser_triggerAction(kWebAction_Cut);
				return true;
			}
		}
	}
	return false;	// no web action generated
}

// send events to the browser
// this currently serves as the generic 'tick' or time slice for the web browser
// so we also handle events such as authorization timeouts
void uiWebBowser_generate_events(bool * eventActivated)
{
	static int s_mouse_last_x = 0x80000000;
	static int s_mouse_last_y = 0x80000000;
	int x, y;
	int x_page, y_page;
	CBox box;

	if (eventActivated)
	{
		*eventActivated = false;
	}

	//setup render coordinates now, based on current window/ screen scaling
	g_ui_Top = g_tex_view_top;
	g_ui_Left = g_tex_view_left;
	convert_browser_to_ui_coords( &g_ui_Left, &g_ui_Top ); 
	g_ui_Bottom = g_tex_view_bottom;
	g_ui_Right = g_tex_view_right;
	convert_browser_to_ui_coords( &g_ui_Right, &g_ui_Bottom ); 

	BuildCBox(&box, g_ui_Left, g_ui_Top, g_ui_Right - g_ui_Left, g_ui_Bottom - g_ui_Top);

	//check if the mouse is in this window
	g_mouse_in_web_window = mouseCollision(&box);
	if (g_mouse_in_web_window)
	{
		// these are relative to top, left corner of screen
		// (i.e. conventional window client coords)
		inpMousePos(&x, &y);

		// figure out location in the browser coordinates
		// (which also has origin at top left)
		x_page = x;
		y_page = y;
		reversePointToScreenScalingFactori(&x_page, &y_page); //convert mouse to screen coords first
		convert_ui_to_browser_coords( &x_page, &y_page );

		if (s_mouse_last_y != y || s_mouse_last_x != x)
		{

			s_mouse_last_y = y;
			s_mouse_last_x = x;

			webBrowser_mouseEvent(kWebEvent_MouseMove, x_page, y_page, 0);
		}
		if (mouseDownHit(&box, MS_RIGHT))
		{
			if (eventActivated)
			{
				*eventActivated = true;
			}
			webBrowser_mouseEvent(kWebEvent_MouseRightButtonDown, x_page, y_page, 0);
			request_input_focus();
		}
		if (mouseUpHit(&box, MS_RIGHT))
		{
			if (eventActivated)
			{
				*eventActivated = true;
			}
			webBrowser_mouseEvent(kWebEvent_MouseRightButtonUp, x_page, y_page, 0);
		}
		if (mouseDownHit(&box, MS_LEFT))
		{
			if (eventActivated)
			{
				*eventActivated = true;
			}
			webBrowser_mouseEvent(kWebEvent_MouseLeftButtonDown, x_page, y_page, 0);
			request_input_focus();
		}
		if (mouseUpHit(&box, MS_LEFT))
		{
			if (eventActivated)
			{
				*eventActivated = true;
			}
			webBrowser_mouseEvent(kWebEvent_MouseLeftButtonUp, x_page, y_page, 0);
		}

		// Mouse Wheel
		{
			int wheelDelta = inpLevel(INP_MOUSEWHEEL);
			if (wheelDelta)
			{
				if (eventActivated)
				{
					*eventActivated = true;
				}
				webBrowser_wheelEvent(wheelDelta*120, x_page, y_page, 0);
			}
			game_state.scrolled = 1;  // stop any more scroll events when the mouse is over this window
		}

		//disregard mouse collisions inside the web browser
		collisions_off_for_rest_of_frame = true;
	}
	else
	{
		// if there is a click outside our frame then we release focus
		if (mouseDownHitNoCol(MS_RIGHT) || mouseDownHitNoCol(MS_LEFT))
		{
			release_input_focus();
		}
	}

	// send keyboard events to the browser if nobody else is claiming focus
	if(!editMode() && uiWebBrowser_has_focus())
	{
		KeyInput* input;

		input = inpGetKeyBuf();
		while(input)
		{
			// convert special key combinations directly to web actions,
			// otherwise make a key event for the browser
			if (!generate_browser_key_action(input))
			{
				generate_browser_key_event(input);
			}

			inpGetNextKey(&input);
		}
	}

	if ( uiWebBrowser_has_focus() )
	{
		webBrowser_tick(100);	// give browser system up to n milliseconds to service events
	}
	else
	{
		// if we don't have keyboard focus we don't want Qt occasionally eating keys on us
		// so use the tick_inactive call to only tick the browser after the key has been consumed
		webBrowser_tick_inactive(25);	// browser is 'inactive' limit event processing
	}

	//PlaySpanStoreLauncher_ServiceHeroAuths();	// check for timouts on any authorizations in progress
}

// render and service native coh ui elements around the browser
// return true if we should exit the menu
static bool uiWebBrowser_handle_native_ui(void)
{
	int bExitMenu = false;
	int screenScalingDisabled = is_scrn_scaling_disabled();
	float screenScaleX, screenScaleY, screenScale;
	float xLoc, yLoc, zLoc;
	int color = 0xFFFFFFFF;

	calc_scrn_scaling(&screenScaleX, &screenScaleY);
	screenScale = MIN(screenScaleX, screenScaleY);

	drawBackground(atlasLoadTexture("background_skyline_light2"));

	set_scrn_scaling_disabled(1);

	// navigation buttons
	xLoc = 400.0f; yLoc = 20; zLoc = 10.0f;
	if( D_MOUSEHIT == drawStdButton( (xLoc)*screenScaleX, yLoc*screenScaleY, zLoc, 50*screenScale, 24*screenScale, color, "Home", 1.f, 0 ) )
	{
		uiWebBrowser_goto_url(get_home_url());
	}
	if( D_MOUSEHIT == drawStdButton( (xLoc+75)*screenScaleX, yLoc*screenScaleY, zLoc, 50*screenScale, 24*screenScale, color, "Back", 1.f, 0 ) )
	{
		webBrowser_triggerAction(kWebAction_Back);
	}
	if( D_MOUSEHIT == drawStdButton( (xLoc+150)*screenScaleX, yLoc*screenScaleY, zLoc, 50*screenScale, 24*screenScale, color, "Forward", 1.f, 0 ) )
	{
		webBrowser_triggerAction(kWebAction_Forward);
	}

	// special pages/test buttons
	xLoc = 400.0f; yLoc = 740.0f; zLoc = 10.0f;
	if( D_MOUSEHIT == drawStdButton( (xLoc)*screenScaleX, yLoc*screenScaleY, zLoc, 80*screenScale, 30*screenScale, color, "Playspan", 1.f, 0 ) )
	{
		//PlaySpanStoreLauncher_EnterStoreFront();
		static const char s_url_google[] = "www.google.com";
		uiWebBrowser_goto_url(s_url_google);
	}
	if( D_MOUSEHIT == drawStdButton( (xLoc+120)*screenScaleX, yLoc*screenScaleY, zLoc, 80*screenScale, 30*screenScale, color, "Google", 1.f, 0 ) )
	{
		static const char s_url_google[] = "www.google.com";
		uiWebBrowser_goto_url(s_url_google);
	}
	if( D_MOUSEHIT == drawStdButton( (xLoc+240)*screenScaleX, yLoc*screenScaleY, zLoc, 80*screenScale, 30*screenScale, color, "Google SSL", 1.f, 0 ) )
	{
		static const char s_url_google_ssl[] = "https://encrypted.google.com/";
		uiWebBrowser_goto_url(s_url_google_ssl);
	}

	// copy/paste
	xLoc = 750.0f; yLoc = 740.0f; zLoc = 10.0f;
	if( D_MOUSEHIT == drawStdButton( (xLoc)*screenScaleX, yLoc*screenScaleY, zLoc, 40*screenScale, 30*screenScale, color, "Copy", 1.f, 0 ) )
	{
		webBrowser_triggerAction(kWebAction_Copy);
	}
	if( D_MOUSEHIT == drawStdButton( (xLoc+60)*screenScaleX, yLoc*screenScaleY, zLoc, 40*screenScale, 30*screenScale, color, "Paste", 1.f, 0 ) )
	{
		webBrowser_triggerAction(kWebAction_Paste);
	}
	if( D_MOUSEHIT == drawStdButton( (xLoc+120)*screenScaleX, yLoc*screenScaleY, zLoc, 40*screenScale, 30*screenScale, color, "Cut", 1.f, 0 ) )
	{
		webBrowser_triggerAction(kWebAction_Cut);
	}


	bExitMenu = drawNextButton( 40*screenScaleX,  740*screenScaleY, 10, screenScale, 0, 0, 0, "CancelString" );

	font(&title_18_thick);
	font_color(CLR_WHITE, SELECT_FROM_UISKIN(CLR_FRAME_BLUE, CLR_FRAME_RED, CLR_WHITE));
	prnt( 40*screenScaleX, 35*screenScaleY,5000, 1.2*screenScale, 1.2*screenScale,"Web Browser");

	// draw percent loaded
	if (g_load_progress<100)
	{
		char text[32];
		sprintf_s(text, sizeof(text), "loading: %d%%", g_load_progress );
		font(&game_14);
		prnt( (1024-155)*screenScaleX, 35*screenScaleY,5000, 1.2*screenScale, 1.2*screenScale, text);
	}
	if (g_is_https)
	{
		font(&game_14);
		prnt( (1024-65)*screenScaleX, 750*screenScaleY,5000, 1.2*screenScale, 1.2*screenScale, "SSL");
	}


	set_scrn_scaling_disabled(screenScalingDisabled);
	return bExitMenu;
}

// HeroWebPartnerCallback
static int webNotifyCallback( EHeroWebNotice notice, void* data )
{
	HeroWebNoticeDataGeneric* pGeneric = (HeroWebNoticeDataGeneric*)data;
	switch (notice)
	{
		case kWebNotice_loadStarted:
			if (cmdAccessLevel()>ACCESS_USER) printf(" loadStarted:\n"); // pGeneric->url points to current frame url, not load target
			g_load_progress = 0;
			g_is_https = false;
			g_is_link_hovered = 0;
			break;

		case kWebNotice_loadProgress:
			g_load_progress =  pGeneric->i1;
			break;

		case kWebNotice_loadFinished:
			if (cmdAccessLevel()>ACCESS_USER) printf( " loadFinished: %s%s\n", pGeneric->i1 ? "" : "*canceled* " , pGeneric->url );
			if (strnicmp(pGeneric->url, "https", 5) == 0)
			{
				g_is_https = true;
			}
			g_load_progress = 100;
			break;
			
		case kWebNotice_repaintRequested:
			// dirty rect (top, left, bottom, right)
//			printf( "repaintRequested (%d,%d,%d,%d)\n", pGeneric->i1, pGeneric->i2 , pGeneric->i3 , pGeneric->i4);
			//@todo would be more efficient to have browser render only the dirty rect provided
			g_repaintRequested = true;
			break;

		case kWebNotice_linkHovered:
//			printf( "linkHovered %s\n", pGeneric->sc1 );
			g_is_link_hovered = pGeneric->sc1 && *pGeneric->sc1;
			break;

		case kWebNotice_cursorChanged:
//			printf( "cursorChanged %d\n", pGeneric->i1 );
			g_web_cursor_shape = pGeneric->i1;
			break;

		case kWebNotice_onHeroAuth:	// PlayNC authorization challenge
			{
				if (cmdAccessLevel()>ACCESS_USER) printf( " onHeroAuth: url: '%s' challenge: '%s'\n", pGeneric->url, pGeneric->sc1 );

				// Send the Steam Auth Session Ticket
				BrowserSendSteamAuthSessionTicket();

				//PlaySpanStoreLauncher_RequestAuthorizationPlayNC(pGeneric->url, pGeneric->sc1);
			}
			break;
		case kWebNotice_onNewFeatureClicked:
//			printf( "newFeatureClicked: %d\n", pGeneric->i1 );
			sendNewFeatureOpenRequest(pGeneric->i1);
			break;
	}
	return true;
}

void uiWebBrowser_render()
{
	onlydevassert( g_web_tex_bind );

	// Ask Hero Browser to render current page if necessary (e.g. dirty/updated)
	// This is currently done with the webkit software rasterizer.
	// We render it now and upload those pixels to the dynamic web texture
	// so the texture is ready when we render 2D sprites/etc later on in the game loop
	// (note that currently this tex update request happens on the render thread outside of a
	// viewport render)
	// @todo When using texGenUpdate the full texture is getting copied to the render thread's 'queue'.
	// This allows us not to flush and stall the render thread as would be necessary if we were
	// going to use the webkit rendered pixel buffer 'in-place'.
	// However this is a very large texture and is expensive to allocate and copy to the queue.
	// A more efficient and stable buffering mechanism can be implemented if required
	// (or we can use the pixels in place and flush the thread)
	if (g_repaintRequested)
	{
//		wtFlush(render_thread);
		g_pixels = webBrowser_render();

		// this call will correctly handle updating the logical page size
		// if the real texture size has been grown to accommodate hardware pow2 dimensions
		texGenUpdateLogical(g_web_tex_bind, g_pixels, GL_RGBA, 0);

		// wait for next notification before re-render of page
		g_repaintRequested = false;
	}

	// Add the browser quad to the sprite list in the correct layer for later rendering
	// when the main viewport sprite batch is handled (see fontRender() and rdrRenderGame().

	// sprites use ui screen panel location (top left origin) for menus and screen coords in game
	if (g_pixels)
	{
		// adjust texture coordinates so that we paint the quad with only the desired portion
		// of the rendered textures (this lets us avoid unwanted borders or scroll areas, etc)
		float u0 = g_tex_view_left/(float)g_tex_width;
		float u1 = g_tex_view_right/(float)g_tex_width;
		float v0 = g_tex_view_top/(float)g_tex_height;
		float v1 = g_tex_view_bottom/(float)g_tex_height;

		// display_sprite uses the texture size to set the base size of the quad so we have to
		// pass in scale values to make the quad the desired size
		float sx = (u1-u0);
		float sy = (v1-v0);

//		display_sprite_tex(g_web_tex_bind, g_panel_left, g_panel_top, g_panel_zlayer, 1.0f, 1.0f, 0xffffffff);
		display_sprite_ex(0, g_web_tex_bind, g_panel_left, g_panel_top, g_panel_zlayer, sx, sy, 0xffffffff,
							0xffffffff, 0xffffffff, 0xffffffff, u0, v0, u1, v1, 0, 0, clipperGetCurrent(), 1, H_ALIGN_LEFT, V_ALIGN_TOP);
	}
}

void uiWebBrowser_set_cursor(void)
{
	// set an appropriate cursor for the user
	if (g_mouse_in_web_window && g_load_progress<75)
	{
		setCursor( "cursor_wait.tga", NULL, FALSE, 11, 10 );
	}
	else if (g_mouse_in_web_window && g_is_link_hovered)
	{
		setCursor( "cursor_point.tga", NULL, FALSE, 2, 2 );
	}
	else if (g_mouse_in_web_window)
	{
		switch (g_web_cursor_shape)
		{
		case kWebCursor_Wait:
		case kWebCursor_Busy:
			setCursor( "cursor_wait.tga", NULL, FALSE, 11, 10 );
			break;
		case kWebCursor_PointingHand:
			setCursor( "cursor_point.tga", NULL, FALSE, 2, 2 );
			break;
		default:
			setCursor( NULL, NULL, FALSE, 2, 2 );
			break;
		}
	}
	else
	{
		setCursor( NULL, NULL, FALSE, 2, 2 );
	}
}

static void oneTimeInit(void)
{
	static int oneTimeOnly = false;
	// make sure we have FPU in full precision or Qt will screw up
	_controlfp_s(&g_saved_fpu_control, 0, 0);	// retrieve current control word
	_controlfp_s(NULL, _PC_64, _MCW_PC);		// set the fpu to full precision
	// initialize the embedded web browser system if this is our first time through
	if (!oneTimeOnly)
	{
		int max_disk_cache_size = 0; // not currently used, 0x2800000;	// 40MB
		webBrowser_system_init( getLocalePlayNC(), max_disk_cache_size, kWebFlag_None, regGetAppKey());

		oneTimeOnly = true;
		// @todo call explicit destroy of the HeroBrowser system when app exits
		// need to change from lazy initialization and instead make explicit system init and shutdown
	}
	// restore FPU precision originally set on entry to browser
	_controlfp_s(NULL, g_saved_fpu_control, _MCW_PC);
}

// for now, page_width and page_height will only apply to the first initialization
// page_width and page_height determine the dimensions of the web page that gets rendered
// the page_view coordinates determine which part of that page we render into the target
// display area.
bool uiWebBrowser_initialize_service(int page_width, int page_height, int page_view_top, int page_view_left, int page_view_bottom, int page_view_right )
{
	// handle initialization of menu as needed
	if (!g_menu_initialized)
	{
		static int oneTimeOnly = false;

		// web page and texture surface size
		g_tex_width = page_width;
		g_tex_height = page_height;

		// portion of the web page that we render on screen into target area
		g_tex_view_top = page_view_top;
		g_tex_view_left = page_view_left;
		g_tex_view_bottom = page_view_bottom;
		g_tex_view_right = page_view_right;

		// allocate the dynamic texture id/bind we will use when rendering
		onlydevassert(!g_web_tex_bind);
		g_web_tex_bind = texGenNew(page_width,page_height,"web_browser");

		oneTimeInit();

		// make sure we have FPU in full precision or Qt will screw up
		_controlfp_s(&g_saved_fpu_control, 0, 0);	// retrieve current control word
		_controlfp_s(NULL, _PC_64, _MCW_PC);		// set the fpu to full precision

		g_repaintRequested = false;
		g_is_https = false;
		g_load_progress = 0;
		g_is_link_hovered = 0;
		g_exit_signal = false;
		g_web_cursor_shape = kWebCursor_Arrow;

		// ask HeroBrowser to create the real web page
		webBrowser_create(g_tex_width, g_tex_height);
		// setup notification callback (after browser creation)
		webBrowser_setPartner( webNotifyCallback );

		request_input_focus();

		g_menu_initialized = 1;
	}

#ifndef FINAL
	{
		// debug test to make sure someone didn't slip the FPU precision down
		// on us behind our back, as that will lead to all kinds of trouble with Qt
		unsigned int fpu_control;
		_controlfp_s(&fpu_control, 0, 0);	// retrieve current control word
		onlydevassert( (fpu_control & _MCW_PC) == _PC_64 );
	}
#endif

	return true;
}

// Since we currently are not supporting independence of the render thread and
// need to reset the FPU mode we have to shutdown with some care.
// returns true if exit is in process and the main service call should just
void uiWebBrowser_exit_handling(void)
{
	release_input_focus();

	// flush the render queue to make sure that gpu will have finished
	// with browser texture (only really necessary if we use web surface in place for render thread)
	wtFlush(render_thread);

	// destroy the browser objects
	webBrowser_destroy();
	webBrowser_tick(200);	// give browser system up to n milliseconds to service events

	// reset our tracking state
	g_menu_initialized = false;
	g_exit_signal = false;
	g_pixels = NULL;
	texGenFree(g_web_tex_bind);
	g_web_tex_bind = NULL;

	g_home_url[0] = 0;		// reset home url for next time we enter

	// exit web browser ui
	setCursor( NULL, NULL, FALSE, 2, 2 );
	// restore FPU precision originally set on entry to browser
	_controlfp_s(NULL, g_saved_fpu_control, _MCW_PC);
}

// main menu service function
// only one function called for the menu per frame so this
// function also needs to handle initialization and exit
void uiWebBrowser_menu_service()
{
	// if exit signal is set handle shutdown
	if (g_exit_signal)
	{
		uiWebBrowser_exit_handling();
		{ // return to the appropriate place
			extern int previous_menu();
			if (previous_menu() == MENU_GAME )
			{
				returnToGame();
			}
			else
			{
				start_menu( previous_menu() );
			}
		}
		return;
	}

	// initialize menu services, create browser if necessary, and place it on our layout
	uiWebBrowser_initialize_service(969, 653, 0, 0, 653, 969);	

	// update browser position
	uiWebBrowser_update_screen_position(26, 50, 500.f); // ui z layer has 0 on the bottom and increasing for layers on top

	// generate event stream for browser
	uiWebBowser_generate_events(NULL);

	// ask the web page to render to a surface (if necessary)
	uiWebBrowser_render();

	// set an appropriate cursor for the user
	uiWebBrowser_set_cursor();

	// Render and service the surrounding 'native' non web page UI elements.
	// This also lets us know when the user hits the button to exit the browser.
	if ( uiWebBrowser_handle_native_ui() )
	{
		g_exit_signal = true;	// set exit signal to begin graceful shutdown
	}
}

void uiWebBrowser_get_web_reset_data(const char * url)
{
	oneTimeInit();

	webDataRetriever_getWebResetData(newFeatures_webDataRetrieveCallback, url);
}

