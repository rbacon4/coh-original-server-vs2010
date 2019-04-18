#ifndef UIWEBSTOREFRAME_H
#define UIWEBSTOREFRAME_H

#include "stdtypes.h"
#include "AccountTypes.h"

typedef struct ShoppingCart
{
	SkuId * items;
	U32 itemCount;
} ShoppingCart;

void webStoreOpenProduct(const char * product);
void webStoreOpenCategory(const char * category);
void webStoreAddToCart(const char * product);
void webStoreAddMultipleToCart(const ShoppingCart * products, U32 first, U32 last);
void webOpenURLNoStore(const char * url);
void webOpenUpgradeToVIP();
void webStoreOpenSupport(void);
void webStoreOpenSupportKB(int kb);
void webStoreOpenNewFeatures();
void webStoreOpenDefault();
void webStoreFrameClose();
void webStoreFrameMenuPopup(F32 x, F32 y, F32 z, F32 sc);
int webStoreFrameWindow();
void webStoreFrameMenuTick();
bool webStoreFrameIsVisibleInFrontEnd();
bool webStoreFrameIsVisibleInGame();
bool webStoreFrameIsVisible();

#endif //UIWEBSTOREFRAME_H
