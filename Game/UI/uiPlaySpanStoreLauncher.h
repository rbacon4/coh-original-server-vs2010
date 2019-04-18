/****************************************************************************
	uiPlaySpanStoreLauncher.h
	
****************************************************************************/
#ifndef __uiPlaySpanStoreLauncher_h__
#define __uiPlaySpanStoreLauncher_h__

void PlaySpanStoreLauncher_InitModule( void );

bool PlaySpanStoreLauncher_IsLoaded();

void PlaySpanStoreLauncher_SetDigest(U32 timeStamp, const char *digest);

void PlaySpanStoreLauncher_RequestAuthorizationPlayNC( const char* url, const char* challenge );
void PlaySpanStoreLauncher_ServiceHeroAuths( void );
void PlaySpanStoreLauncher_ReceiveAuthKeyResponse( int request_key, const char* auth_key );

bool PlaySpanStoreLauncher_EnterStoreFront();
bool PlaySpanStoreLauncher_ShowShoppingCart();
bool PlaySpanStoreLauncher_ShowProductInStore( const char* sku );
bool PlaySpanStoreLauncher_AddProductToShoppingCart( const char* sku );
bool PlaySpanStoreLauncher_AddProductListToShoppingCart( const char** skuArray, int numItems );
bool PlaySpanStoreLauncher_ShowCategoryInStore( const char* storeCategory, const char* storeSubCategory /* NULL OK */ );
bool PlaySpanStoreLauncher_ManageAccount();
bool PlaySpanStoreLauncher_NCSoftHelp( void );
bool PlaySpanStoreLauncher_NCSoftHelpKB( int kb );
bool PlaySpanStoreLauncher_UpgradeToVIP();
bool PlaySpanStoreLauncher_NewFeatures();
bool PlaySpanStoreLauncher_NewFeaturesUpdate();

#endif	// __uiPlaySpanStoreLauncher_h__
