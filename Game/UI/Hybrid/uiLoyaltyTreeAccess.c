#include "uiLoyaltyTreeAccess.h"
#include "uiHybridMenu.h"
#include "uiWindows.h"
#include "uiUtil.h"
#include "wdwbase.h"
#include "uiUtilGame.h"
#include "uiOptions.h"

#define CENTER_X 25.f
#define CENTER_Y 24.5f

int loyaltyTreeAccessWindow()
{
	float x, y, z, wd, ht, scale;
	int color, bcolor;
	static HybridElement sButtonTreeAccess = {0, NULL, "loyaltyTreeButton", "icon_ig_rewardsaccess_color_full"};

	if (optionGet(kUO_HideLoyaltyTreeAccessButton))
	{
		return 0;
	}

	// Do everything common windows are supposed to do.
	if ( !window_getDims( WDW_LOYALTY_TREE_ACCESS, &x, &y, &z, &wd, &ht, &scale, &color, &bcolor ))
		return 0;

	//draw store icon
	if (D_MOUSEHIT == drawHybridButtonWindow(&sButtonTreeAccess, x+CENTER_X*scale, y+CENTER_Y*scale, z, scale, CLR_WHITE, HB_SHRINK_OVER, WDW_LOYALTY_TREE_ACCESS))
	{
		window_openClose(WDW_LOYALTY_TREE);
	}

	drawFrame( PIX3, R22, x, y, z-2.f, wd, ht, scale, color, bcolor );

	return 0;
}
