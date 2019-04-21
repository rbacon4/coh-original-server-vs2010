#include "uiWebStoreFrame.h"
#include "wdwbase.h"
#include "player.h"
#include "uiWindows.h"
#include "uiUtilGame.h"
#include "uiUtil.h"
#include "uiHybridMenu.h"
#include "uiGame.h"
#include "textureatlas.h"
#include "sprite_base.h"
#include "cmdcommon.h"
#include "AccountData.h"
#include "earray.h"
#include "sprite_font.h"
#include "sprite_text.h"
#include "Cbox.h"
#include "uiInput.h"
#include "dbclient.h"
#include "cmdgame.h"
#include "entity.h"
#include "entPlayer.h"
#include "uiDialog.h"
#include "uiWebBrowser.h"
#include "uiPlaySpanStoreLauncher.h"
#include "rt_tune.h"
#include "..\libs\HeroBrowser\HeroBrowser.h"
#include "sound.h"
#include "uiDialog.h"
#include "inventory_client.h"
#include "uiDialog.h"

#define WSF_WEB_WIDTH	799.f  //width of the viewable web page
#define WSF_WEB_HEIGHT	598.f //height of the viewable web page
#define WSF_WIDTH		(WSF_WEB_WIDTH+HBPOPUPFRAME_LEFT_OFFSET+HBPOPUPFRAME_RIGHT_OFFSET)  //full width of the window
#define WSF_HEIGHT		(WSF_WEB_HEIGHT+HBPOPUPFRAME_TOP_OFFSET+HBPOPUPFRAME_BOTTOM_OFFSET) //full height of the window
#define WSF_ICONSPACE	((WSF_WIDTH-100.f) / 4.f)

#define WEB_BUTTON_CLICK_DELAY 30.f;

// Variables to control the size of the web page render surface
// and the portion of it we display in the window.
// These are variables so we can use the tweak menu to come up with good
// values.

#if 0
// page dimensions are currently larger than display viewport because the store
// web pages are not all the same size and we need to avoid the scroll bars coming
// up and causing corruption
static int g_wsf_page_width = 808;				
static int g_wsf_page_height = 664;

// portion of the page we render, also inset to eliminate black borders, especially at corners with rounded frame
static int g_wsf_page_view_top = 5;
static int g_wsf_page_view_left = 9;
static int g_wsf_page_view_bottom = 595;
static int g_wsf_page_view_right = 799;
#else
#define PLAYSPAN_PAGE_WIDTH		800 //786
#define PLAYSPAN_PAGE_HEIGHT	600 //587
static int g_wsf_page_view_left = 0;
static int g_wsf_page_view_top = 0;
static int g_wsf_page_view_right = PLAYSPAN_PAGE_WIDTH;
static int g_wsf_page_view_bottom = PLAYSPAN_PAGE_HEIGHT;

// page dimensions are a pixel larger in each direction to avoid scrollbars for being off by only a single pixel
static int g_wsf_page_width = PLAYSPAN_PAGE_WIDTH + 1;				
static int g_wsf_page_height = PLAYSPAN_PAGE_HEIGHT + 1;
#endif

// amount we offset the rendering of the web page panel from the window anchor (and frame)
// this lets us inset the page rendering to eliminate the black border on top and left, especially at corners with rounded frame
static int g_wsf_screen_offset_x = 7;
static int g_wsf_screen_offset_y = 43;

static int gWebStoreMenuVisible;  //indicates whether the store is visible in the front-end (since we don't have window management)
static int showStoreButtons;
static int init;
static F32 timeToActivateWebButtons;

static void webStoreInitialize(void)
{
	// initialize menu services and create browser if necessary
	uiWebBrowser_initialize_service( g_wsf_page_width, g_wsf_page_height, g_wsf_page_view_top, g_wsf_page_view_left, g_wsf_page_view_bottom, g_wsf_page_view_right );
	init = 1;

	// tweak menu page size/view location tuning
#ifndef FINAL
	{
		static int s_init_once;
		if (!s_init_once)
		{
			tunePushMenu("Web Store");
			tuneInteger("Page Width", &g_wsf_page_width, 1, 16, 1024, NULL);
			tuneInteger("Page Height", &g_wsf_page_height, 1, 16, 1024, NULL);

			tuneInteger("Page View Top", &g_wsf_page_view_top, 1, 0, 1024, NULL);
			tuneInteger("Page View Left", &g_wsf_page_view_left, 1, 0, 1024, NULL);
			tuneInteger("Page View Bottom", &g_wsf_page_view_bottom, 1, 0, 1024, NULL);
			tuneInteger("Page View Right", &g_wsf_page_view_right, 1, 0, 1024, NULL);

			tuneInteger("Page Screen Offset X", &g_wsf_screen_offset_x, 1, 0, 256, NULL);
			tuneInteger("Page Screen Offset Y", &g_wsf_screen_offset_y, 1, 0, 256, NULL);
			tunePopMenu();
			s_init_once = true;
		}
	}
#endif
}

void webStoreFrame(int show); //fwd declaration

void webStoreOpenProduct(const char * product)
{
	//open window and init
	webStoreFrame(1);

	//set url based on product
	//PlaySpanStoreLauncher_ShowProductInStore(product);
}

void webStoreOpenCategory(const char * category)
{
	//open window and init
	webStoreFrame(1);

	//set url based on category
	//PlaySpanStoreLauncher_ShowCategoryInStore(category, 0);
}

void webStoreAddToCart(const char * product)
{
	//open window and init
	webStoreFrame(1);

	//set url based on product
	//PlaySpanStoreLauncher_AddProductToShoppingCart(product);
}

void webStoreAddMultipleToCart(const ShoppingCart * products, U32 first, U32 last)
{
	U32 i;
	U32 numSkus = last - first;
	char skus[256] = "";

	//open window and init
	webStoreFrame(1);

	//set url with products
	devassert(numSkus && products->itemCount >= last);
	strcat(skus, skuIdAsString(products->items[first]));
	for (i = 1; i < numSkus; ++i)
	{
		strcat(skus, ",");
		strcat(skus, skuIdAsString(products->items[first+i]));
	}
	//PlaySpanStoreLauncher_AddProductToShoppingCart( skus );
}

void webOpenURLNoStore(const char * url)
{
	//open window and init
	webStoreFrame(1);

	showStoreButtons = false;
	uiWebBrowser_goto_url(url);
}

void webOpenUpgradeToVIP()
{
	//open window and init
	webStoreFrame(1);

	showStoreButtons = false;
	//PlaySpanStoreLauncher_UpgradeToVIP();
}

void webStoreOpenSupport(void)
{
	//open window and init
	webStoreFrame(1);

	//PlaySpanStoreLauncher_NCSoftHelp();
}

void webStoreOpenSupportKB(int kb)
{
	//open window and init
	webStoreFrame(1);

	//PlaySpanStoreLauncher_NCSoftHelpKB(kb);
}

void webStoreOpenNewFeatures()
{
	//open window and init
	webStoreFrame(1);

	//if (!PlaySpanStoreLauncher_NewFeatures())
	//{
		dialogStd(DIALOG_OK, "NewFeaturesNotAvailable", NULL, NULL, NULL, NULL, 0);
		webStoreFrame(0);
	//}
}

void webStoreOpenDefault()
{
	//open window and init
	webStoreFrame(1);

	//set default url
	//PlaySpanStoreLauncher_EnterStoreFront();
}

void webStoreFrameClose()
{
	if (init)
	{
		//web browser cleanup
		uiWebBrowser_exit_handling();
		init = 0;
	}

	if (current_menu() == MENU_GAME)
	{
		window_setMode(WDW_WEB_STORE, WINDOW_SHRINKING);
	}
	else
	{
		gWebStoreMenuVisible = 0;
	}
}

static int drawBrowserButtonFrame(F32 x, F32 y, F32 z, F32 sc)
{
	AtlasTex * storeButton = atlasLoadTexture("icon_store_back");  //for sizing
	F32 buttonScale = 0.75f;
	F32 xwalk = x + 25.f*sc;
	F32 ywalk = y + 6.f*sc + storeButton->height*sc*buttonScale/2.f;
	F32 zwalk = z + 3.f;
	F32 buttonWd = 200.f;
	int flagshb = 0;
	static HybridElement sButtonBack = {0, NULL, NULL, "icon_store_back"};
	static HybridElement sButtonClose = {0, NULL, NULL, "icon_store_close"};
	static HybridElement sButtonHome = {0, "WebHome", NULL, "icon_store_home"};
	static HybridElement sButtonCart = {0, "WebCart", NULL, "icon_store_shoppingcart"};
	static HybridElement sButtonSupport = {0, "WebSupport", NULL, "icon_store_support"};
	static HybridElement sButtonAccount = {0, "WebMyAccount", NULL, "icon_store_myaccount"};
	
	if (timeToActivateWebButtons > 0.f)
	{
		flagshb |= HB_DISABLED;
		timeToActivateWebButtons -= TIMESTEP;
	}

	//draw back button
	if (D_MOUSEHIT == drawHybridButton(&sButtonBack, xwalk, ywalk, zwalk, buttonScale*sc, CLR_WHITE, HB_DRAW_BACKING | flagshb))
	{
		sndPlay("N_SelectSmall", SOUND_GAME);
		uiWebBrowser_go_back();
		timeToActivateWebButtons = WEB_BUTTON_CLICK_DELAY;
	}
	//draw home button
	xwalk += 110.f*sc;
	if (showStoreButtons)
	{
		if (D_MOUSEHIT == drawHybridBarButton(&sButtonHome, xwalk, ywalk, zwalk, buttonWd, buttonScale*sc, 0, 0))
		{
			sndPlay("N_SelectSmall", SOUND_GAME);
			//PlaySpanStoreLauncher_EnterStoreFront();
		}
	}
	//draw shoppingcart button
	xwalk += WSF_ICONSPACE*sc;
	if (showStoreButtons)
	{
		if (D_MOUSEHIT == drawHybridBarButton(&sButtonCart,xwalk, ywalk, zwalk, buttonWd, buttonScale*sc, 0, 0))
		{
			sndPlay("N_SelectSmall", SOUND_GAME);
			//PlaySpanStoreLauncher_ShowShoppingCart();
		}
	}
	//draw storesupport button
	xwalk += WSF_ICONSPACE*sc;
	if (showStoreButtons)
	{
		if (D_MOUSEHIT == drawHybridBarButton(&sButtonSupport, xwalk, ywalk, zwalk, buttonWd, buttonScale*sc, 0, 0))
		{
			sndPlay("N_SelectSmall", SOUND_GAME);
			//PlaySpanStoreLauncher_NCSoftHelp();
		}
	}
	//draw myaccount button
	xwalk += WSF_ICONSPACE*sc;
	if (showStoreButtons)
	{
		if (D_MOUSEHIT == drawHybridBarButton(&sButtonAccount, xwalk, ywalk, zwalk, buttonWd, buttonScale*sc, 0, 0))
		{
			sndPlay("N_SelectSmall", SOUND_GAME);
			//PlaySpanStoreLauncher_ManageAccount();
		}
	}
	//draw close button
	xwalk += 110.f*sc;
	if (D_MOUSEHIT == drawHybridButton(&sButtonClose, xwalk, ywalk, zwalk, buttonScale*sc, CLR_WHITE, HB_DRAW_BACKING))
	{
		sndPlay("N_Deselect", SOUND_GAME);
		webStoreFrameClose();
		return 0;
	}
	return 1;
}

static void webStoreFrameRender(F32 x, F32 y, F32 z, F32 sc, bool * eventActivated)
{
	//draw web browser
	// update browser position
	uiWebBrowser_update_screen_position((int)x + g_wsf_screen_offset_x, (int)y + g_wsf_screen_offset_y, z); // ui z layer has 0 on the bottom and increasing for layers on top

	// generate event stream for browser
	uiWebBowser_generate_events(eventActivated);

	// ask the web page to render to a surface (if necessary)
	uiWebBrowser_render();

	// set an appropriate cursor for the user
	uiWebBrowser_set_cursor();
}

void webStoreFrameMenuPopup(F32 x, F32 y, F32 z, F32 sc)
{
	int lastSSD = is_scrn_scaling_disabled();
	set_scrn_scaling_disabled(1);

	//draw frame
	drawHybridPopupFrame(x, y, z, (WSF_WIDTH)*sc, (WSF_HEIGHT)*sc);

	//draw buttons
	if (drawBrowserButtonFrame(x, y, z, sc))
	{
		//draw contents
		webStoreFrameRender(x, y, z, sc, NULL);
	}

	set_scrn_scaling_disabled(lastSSD);

	//block the rest of the input
	collisions_off_for_rest_of_frame = true;
}

static int webStoreWindowOpen(void)
{
	AccountServerStatus status = inventoryClient_GetAcctAuthoritativeState();

	switch (status)
	{
	case ACCOUNT_SERVER_UP:
		webStoreOpenDefault();
		return 1;
		break;
	case ACCOUNT_SERVER_SLOW:
		dialogStd(DIALOG_OK, "paragonstoreSlowButton", 0, 0, 0, 0, 0);
		break;
	case ACCOUNT_SERVER_DOWN:
		dialogStd(DIALOG_OK, "paragonstoreDownButton", 0, 0, 0, 0, 0);
		break;
	}
	return 0;
}

int webStoreFrameWindow()
{
	float x, y, z, wd, ht, scale;
	int color, bcolor;

	Entity * e = playerPtr();
	int mode = window_getMode(WDW_WEB_STORE);

	//in case we got to this window without initializing first
	if (!init && (mode == WINDOW_DISPLAYING || mode == WINDOW_GROWING))
	{
		if (!webStoreWindowOpen())
		{
			//account server was down, so close the window if it was open previously
			webStoreFrameClose();
			return 0;
		}
	}

	// Do everything common windows are supposed to do.
	if ( !window_getDims( WDW_WEB_STORE, &x, &y, &z, &wd, &ht, &scale, &color, &bcolor ) || !e)
		return 0;
	if (window_getMode(WDW_WEB_STORE) == WINDOW_DISPLAYING)
	{
		window_setDims(WDW_WEB_STORE, x, y, WSF_WIDTH, WSF_HEIGHT );
	}
	
	//store is drawn at a 1.0 scale and UI scale will not affect it
	//draw frame
	drawHybridPopupFrame(x, y, z, WSF_WIDTH, WSF_HEIGHT);

	//draw buttons
	if (drawBrowserButtonFrame(x, y, z, 1.f))
	{
		//draw contents only if the window is showing
		if ((mode == WINDOW_DISPLAYING || mode == WINDOW_GROWING))
		{
			bool eventActivated;
			webStoreFrameRender(x, y, z, 1.f, &eventActivated);
			if (eventActivated)
			{
				window_bringToFront( WDW_WEB_STORE );
			}
		}
	}

	return 0;
}

static void webStoreFrame(int show)
{
	if (show)
	{
		// initialize menu services and create browser if necessary
		webStoreInitialize();
		timeToActivateWebButtons = 0;
		showStoreButtons = true;

		if (current_menu() == MENU_GAME)
		{
			window_setMode(WDW_WEB_STORE, WINDOW_GROWING);
		}
		else
		{
			gWebStoreMenuVisible = 1;
		}

	}
	else
	{
		webStoreFrameClose();
	}
}

bool webStoreFrameIsVisibleInFrontEnd()
{
	return gWebStoreMenuVisible;
}

bool webStoreFrameIsVisibleInGame()
{
	return window_getMode(WDW_WEB_STORE) == WINDOW_DISPLAYING;
}

bool webStoreFrameIsVisible()
{
	return webStoreFrameIsVisibleInGame() || webStoreFrameIsVisibleInFrontEnd();
}

void webStoreFrameMenuTick()
{
	if (gWebStoreMenuVisible)
	{
		//put the store in the middle of the screen
		F32 x = DEFAULT_SCRN_WD / 2.f;
		F32 y = DEFAULT_SCRN_HT / 2.f;
		applyToScreenScalingFactorf(&x, &y);

		webStoreFrameMenuPopup(x-WSF_WIDTH/2.f, y-WSF_HEIGHT/2.f, 6000.f, 1.f);
	}
	else if (init)
	{
		//web browser cleanup
		uiWebBrowser_exit_handling();
		init = 0;
	}
}
