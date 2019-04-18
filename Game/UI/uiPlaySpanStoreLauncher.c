/****************************************************************************
	uiPlaySpanStoreLauncher.c
	
****************************************************************************/

#include "uiPlaySpanStoreLauncher.h"
#include "uiWebBrowser.h"
#include "..\libs\HeroBrowser\HeroBrowser.h"
#include "assert.h"
#include "AccountCatalog.h"
#include "uiLogin.h"
#include "Entity.h"
#include "Player.h"
#include "character_base.h"
#include "textparser.h"
#include "earray.h"
#include "EString.h"
#include "LoadDefCommon.h"
#include "crypt.h"
#include "authclient.h"
#include "AppLocale.h"
#include "dbclient.h"
#include "timing.h"
#include "cmdgame.h"
#include "uiNet.h"
#include "inventory_client.h"

#define PLAYSPAN_DIGEST_MAX (128+1)

//---------------------------------------------------------------------------
//	Data types
//---------------------------------------------------------------------------
typedef struct _tPlaySpanStoreLauncherNameValuePair
{
	const char* valName;
	const char* value;
} tPlaySpanStoreLauncherNameValuePair;

typedef enum {
	kStoreFrontHomePageId			= 1,	
	kStoreFrontCategoryViewPageId,
	kStoreFrontItemViewPageId,
	kStoreFrontAddToCartPageId,
	kStoreFrontShowShoppingCart,
	kPlayNcManageAccountPageId,
	kPlayNcSupportPagePageId,
	kPlayNcSupportPagePageIdDE,
	kPlayNcSupportPagePageIdFR,
	kPlayNcUpgradeToVIPPageId,
	kCohNewFeaturesPageId,
	kCohNewFeaturesUpdatePageId,
} tStoreFrontPageId;

typedef enum {
	kWebStoreAuth_NoTimeStampMD5,
	kWebStoreAuth_TimeStampSHA1
} tWebStoreAuthType;

//---------------------------------------------------------------------------
//	Static function declarations
//---------------------------------------------------------------------------
static const char*			getUrlFormatForPageId( const char* pageId );
static void					addToURL( char** anEStrBuffer, 
								int* pNumArgsAdded,
								const char* argName,
								const char* argValue );
static bool					addFragmentToURL( char** anEStrBuffer, 
								const char* fragment );
static bool					buildStoreURL(
								tStoreFrontPageId pageId,
								int numDataVals,
								tPlaySpanStoreLauncherNameValuePair* dataVals,
								const char* tail,
								char* urlBuffer,
								size_t urlBuffSz );
static bool					gotoPlaySpanStorePage( 
								tStoreFrontPageId pageId, 
								int numDataVals,
								tPlaySpanStoreLauncherNameValuePair* dataVals,
								const char* tail );
static const char*			getUrlForPlaySpanStorePage( tStoreFrontPageId pageId );
static const char*			getPlaySpanDomain( void );
static const char*			getPlaySpanCatalogId( void );
static void					getPlaySpanDigest(char * digest, size_t digest_size);
static const char*			skipPrefix( const char* aStr, const char* possiblePrefix );
static const char*			getCharArchetype( void );
static const char*			getCharLevel( void );
static const char*			getLocale( void );
static const char*			getShard( void );
static bool					userHasVIPStatus( void );
static bool					userHasLL5Status( void );
static tWebStoreAuthType	getStoreAuthType( void );
static void					deDupeCommaSeparatedList( char* listBuffer /* may change the data!!! */ );

static void					requestPlayNCAuthKey( const char* URL, 
									const tPlaySpanStoreLauncherNameValuePair* digestData[], 
									int numDataItems );
static void					clearChallengeRequestSlot( int challengeSlot );
static void					storeChallengeRequestInSlot( int challengeSlot, int request_key, 
									const char* url, 
									const tPlaySpanStoreLauncherNameValuePair* digestData[], 
									int numDataItems );
static bool					addProductSKUsToShoppingCart( const char* skuListCSV );

#define SafeStringPtr(_ptr_) (((_ptr_)==NULL) ? "" : (_ptr_))

//---------------------------------------------------------------------------
//	Static global data
//---------------------------------------------------------------------------

static char		s_PlaySpanDigest[PLAYSPAN_DIGEST_MAX];
static size_t	s_PlaySpanDigestLength;
static U32		s_PlaySpanDigestTimeStamp;	// the timestamp used for the current digest

#define MAX_PENDING_CHALLENGED_URLS 4
typedef struct _tChallenge {
	bool	request_active;			// this is an active request in progress
	int		request_key;			// unique id assigned to request
	char*	url;					// url request (not currently used for anything of import)
	U32		request_time;			// time request was received so that we can timout requests
	tPlaySpanStoreLauncherNameValuePair	**challengeParams;
} tChallenge;

tChallenge		s_ChallengedURLRequests[MAX_PENDING_CHALLENGED_URLS] = { 0 };
static int		s_nextChallengeID = 0;
#define			GetChallengeRequestSlot( _request_id_ )	(( _request_id_ ) % MAX_PENDING_CHALLENGED_URLS )
#define			kHeroAuthTimoutSeconds	45			// amount of time before we timeout/abandon a HeroAuth request, Account server is probably not reachable

/****************************************************************************
	PlaySpanStoreLauncher_InitModule

****************************************************************************/
void PlaySpanStoreLauncher_InitModule( void )
{
	static bool s_IsInitialized = false;

	// not really needed, but makes me feel better
	memset( s_ChallengedURLRequests, 0, sizeof(s_ChallengedURLRequests) );
	
	s_IsInitialized = true;
	
	// Testing deDupeCommaSeparatedList()
	//#if ! defined(FINAL)
	#if 0
	{
		int i;
		char buffer[256];
		const char* testCases[] = {
			"CUCPMPMA,CUCPMPBE,CUCPMPBJ,CUCPMPBS,CUCPMPBB,CUCPMPBC,CUCPMPBC",	// remove from the end
			"CUCPMPMA,CUCPMPMA,CUCPMPBE,CUCPMPBJ,CUCPMPBS,CUCPMPBB,CUCPMPBC,CUCPMPBC",	// remove from the beginning
			"CUCPMPMA,CUCPMPBE,CUCPMPBJ,CUCPMPMA,CUCPMPBS,CUCPMPBB,CUCPMPBC",	// dupe in the middle of the string
			"CUCPMPMA,CUCPMPBE,CUCPMPBJ,CUCPMPMA,CUCPMPMA,CUCPMPBS,CUCPMPBB,CUCPMPBC",	// 2 dupes, next to each other
			"CUCPMPMA,CUCPMPBE,CUCPMPBJ,CUCPMPMA,CUCPMPMA,CUCPMPBS,CUCPMPBB,CUCPMPBC,CUCPMPMA"	// lots of dupes
		};
		for ( i=0; i < ARRAY_SIZE(testCases); i++ ) {
			strcpy_s( buffer, ARRAY_SIZE(buffer), testCases[i] );
			deDupeCommaSeparatedList( buffer );
			assert( strcmp( buffer, "CUCPMPMA,CUCPMPBE,CUCPMPBJ,CUCPMPBS,CUCPMPBB,CUCPMPBC" ) == 0 );
		}	
	}
	#endif
	
}

/****************************************************************************
	PlaySpanStoreLauncher_IsLoaded

****************************************************************************/
bool PlaySpanStoreLauncher_IsLoaded( void )
{
	const AccountStoreAccessInfo* info = accountCatalog_GetStoreAccessInfo();

	return (( info->playSpanDomain != NULL ) && ( *info->playSpanDomain != '\0' ));
}

/****************************************************************************
	PlaySpanStoreLauncher_EnterStoreFront

	Sample URL:
		https://store-qa.playspan.com/ps-primary-web/ncsf/app/shop.jsp?
			userid=nirajdesh@gmail.com
			&
			sn=NCSF
			&
			digest=0cdb3184016bd5cf78d97cf30934f6a5
****************************************************************************/
bool PlaySpanStoreLauncher_EnterStoreFront()
{
	return gotoPlaySpanStorePage( kStoreFrontHomePageId, 0, NULL, NULL );
}

/****************************************************************************
	PlaySpanStoreLauncher_ShowProductInStore

	Sample URL:
		https://store-qa.playspan.com/ps-primary-web/ncsf/app/buynow.jsp?
			userid=nirajdesh@gmail.com
			&
			sn=NCSF
			&
			digest=0cdb3184016bd5cf78d97cf30934f6a5
			&
			r.skulist=COESREF1
****************************************************************************/
bool PlaySpanStoreLauncher_ShowProductInStore( const char* sku )
{
	tPlaySpanStoreLauncherNameValuePair selections[] = {
		{ "r.skulist", sku }
	};
	return gotoPlaySpanStorePage( 
				kStoreFrontItemViewPageId, ARRAY_SIZE(selections), selections, NULL );
}

/****************************************************************************
	deDupeCommaSeparatedList

	Looks for duplicate comma-delimited segments in the string and removes
	them (by shifting the contents).
****************************************************************************/
void deDupeCommaSeparatedList( char* listBuffer /* may change the data!!! */ )
{
	char* pCurrSeg = NULL;
	int currSegLen = 0;
	int i = -1;
	char currChar = -1;
	int listLen = (int)strlen(listBuffer);

	while ( currChar != '\0' ) {		// we don't stop until AFTER we've seen the null terminator
		currChar = listBuffer[++i];
		if ( pCurrSeg == NULL ) {
			//
			// Remember where the current segment started
			//
			pCurrSeg = ( listBuffer + i );
			while ( *pCurrSeg == ' ' ) pCurrSeg++;
			currSegLen = 0;
		}
		if (( currChar == ',' ) || ( currChar == '\0' )) {
			//
			//	Hit a delimiter or the end of the string. See if the current
			//	segment can be found in the portion of the string we've already
			//	looked at.
			//
			const char* pFind;
			listBuffer[i] = '\0';	// temporarily, so we can pass pCurrSeg to strstr().
			pFind = strstr( listBuffer, pCurrSeg );
			if (( pFind != NULL ) &&
				( pFind < pCurrSeg ) && 
				(( *( pFind + currSegLen ) == ',' ) || ( *( pFind + currSegLen ) == ' ' ))) {
				//
				// Found a copy
				//
				int segSz = currSegLen;
				if ( currChar == ',' ) segSz++; // also remove the divider comma
				memmove( pCurrSeg, ( pCurrSeg + segSz ), (( listLen - i ) + 1 ) );
				listLen -= segSz;
				i -= segSz;
			} else {
				listBuffer[i] = currChar;	// put it back the way it was
			}
			pCurrSeg = NULL;
		} else {
			currSegLen++;
		}
	}
	if (( listLen != 0 ) && ( *( listBuffer + listLen - 1 ) == ',' )) {
		// Remove a side effect of the way we walk the string segs. If the
		// last segment was a dupe and has been removed, we'll be left with
		// a comma on the end of the string.
		listBuffer[--listLen] = '\0';
	}
}

/****************************************************************************
	addProductSKUsToShoppingCart

	skuListCSV is either a single SKU or a comma-separated list of SKUs.
****************************************************************************/
bool addProductSKUsToShoppingCart( const char* skuListCSV )
{
	char* skuListNew;
	strdup_alloca( skuListNew, skuListCSV );
	deDupeCommaSeparatedList( skuListNew );
	
	{
		tPlaySpanStoreLauncherNameValuePair selections[] = {
			{ "r.sku", skuListNew }
		};
		return gotoPlaySpanStorePage( 
				kStoreFrontAddToCartPageId, ARRAY_SIZE(selections), selections, NULL );
	}
}

/****************************************************************************
	PlaySpanStoreLauncher_AddProductToShoppingCart

****************************************************************************/
bool PlaySpanStoreLauncher_AddProductToShoppingCart( const char* sku )
{
	return addProductSKUsToShoppingCart( sku );
}

/****************************************************************************
	PlaySpanStoreLauncher_AddProductListToShoppingCart

****************************************************************************/
bool PlaySpanStoreLauncher_AddProductListToShoppingCart( const char** skuArray, int numItems )
{
	int i;
	size_t buffSz = 0;
	char* skuCSV = NULL;

	//
	//	Make a comma separated list.
	//
	for ( i=0; i < numItems; i++ ) {
		buffSz += ( strlen(skuArray[i]) + 1 );	// room for the item, plus a comma
	}
	buffSz++;
	
	skuCSV		= (char*)_alloca( buffSz );
	skuCSV[0]	= '\0';
	
	for ( i=0; i < numItems; i++ ) {
		if ( i > 0 ) {
			strcat_s( skuCSV, buffSz, "," );
		}
		strcat_s( skuCSV, buffSz, skuArray[i] );
	}
	
	return addProductSKUsToShoppingCart( skuCSV );
}

/****************************************************************************
	PlaySpanStoreLauncher_AddProductToShoppingCart

****************************************************************************/
bool PlaySpanStoreLauncher_ShowShoppingCart( void )
{
	return gotoPlaySpanStorePage( 
				kStoreFrontShowShoppingCart, 0, NULL, NULL );
}

/****************************************************************************
	PlaySpanStoreLauncher_ShowCategoryInStore

	Sample URL:
		https://store-qa.playspan.com/ps-primary-web/ncsf/app/viewcategory.jsp?
			userid=nirajdesh@gmail.com
			&
			sn=NCSF
			&
			digest=0cdb3184016bd5cf78d97cf30934f6a5&
			r.category=1000
****************************************************************************/
bool PlaySpanStoreLauncher_ShowCategoryInStore( const char* storeCategory, const char* storeSubCategory /* NULL OK */ )
{
	tPlaySpanStoreLauncherNameValuePair selections[] = {
		{ "r.category",		storeCategory },
		/*{ "sub-category",	storeSubCategory },*/
	};
	return gotoPlaySpanStorePage( 
				kStoreFrontCategoryViewPageId, ARRAY_SIZE(selections), selections, NULL );
}

bool PlaySpanStoreLauncher_ManageAccount()
{
	return gotoPlaySpanStorePage( kPlayNcManageAccountPageId, 0, NULL, NULL );
}

bool PlaySpanStoreLauncher_NCSoftHelp( void )
{
	return PlaySpanStoreLauncher_NCSoftHelpKB( -1 );
}

bool PlaySpanStoreLauncher_NCSoftHelpKB( int kb )
{
	int locale = getCurrentLocale();
	const char* format;
	tStoreFrontPageId pageId;
	char* estrTail = estrTemp();
	bool ret;

	switch (locale)
	{
	case LOCALE_ID_GERMAN:
		format = "#/de/answers/kbdetail/a_id/%d";
		pageId = kPlayNcSupportPagePageIdDE;
		break;
	case LOCALE_ID_FRENCH:
		format = "#/fr/answers/kbdetail/a_id/%d";
		pageId = kPlayNcSupportPagePageIdFR;
		break;
	case LOCALE_ID_ENGLISH:
	default:
		format = "#/en/answers/kbdetail/a_id/%d";
		pageId = kPlayNcSupportPagePageId;
		break;
	}
	
	if (kb != -1)
	{
		estrPrintf(&estrTail, format, kb);
	}

	ret = gotoPlaySpanStorePage( pageId, 0, NULL, estrTail );

	estrDestroy(&estrTail);

	return ret;
}

bool PlaySpanStoreLauncher_UpgradeToVIP()
{
	return gotoPlaySpanStorePage( kPlayNcUpgradeToVIPPageId, 0, NULL, NULL );
}

bool PlaySpanStoreLauncher_NewFeatures()
{
	return gotoPlaySpanStorePage( kCohNewFeaturesPageId, 0, NULL, NULL );
}

bool PlaySpanStoreLauncher_NewFeaturesUpdate()
{
	char url[128];
	if (buildStoreURL (kCohNewFeaturesUpdatePageId, 0, 0, 0, url, 128))
	{
		uiWebBrowser_get_web_reset_data(url);
		return true;
	}
	return false;
}

const char* skipPrefix( const char* aStr, const char* possiblePrefix )
{
	int ofs = 0;
	const char* pStr = aStr;
	const char* pPfx = possiblePrefix;

	//
	// If we find possiblePrefix in aStr return the location
	// in aStr after the prefix. Otherwise, just return aStr.
	//
	
	while (( *pStr != '\0' ) &&
			( *pPfx != '\0' ) &&
			( *pStr == *pPfx )) {
		ofs++;
		pStr++;
		pPfx++;
	}
	return ( *pPfx == '\0' ) ? ( aStr + ofs ) : aStr;
}

const char* getLoginUI( void )
{
	return g_achAccountName;
}

const char* getCharArchetype( void )
{
	Entity* ent = playerPtr();
	if (( ent != NULL ) && ( ent->id != 0 ) && ( ent->pchar )) {
		return skipPrefix( ent->pchar->pclass->pchName, "Class_" );
	}
	return NULL;
}

const char* getCharLevel( void )
{
	static char levelStr[32];
	Entity* ent = playerPtr();
	if (( ent != NULL ) && ( ent->id != 0 ) && ( ent->pchar )) {
		sprintf_s( levelStr, ARRAY_SIZE(levelStr), "%d", ent->pchar->iLevel );
		return levelStr;
	}
	return NULL;
}

const char* getLocale( void )
{
	const AccountStoreAccessInfo* info = accountCatalog_GetStoreAccessInfo();
	int storeLanguage = LOCALE_ENGLISH;
	if (! ( info->playSpanStoreFlags & STOREFLAG_NO_LOCALIZATION )) {
		storeLanguage = getCurrentLocale();
	}
	return  locGetAlpha2( storeLanguage );
}

const char* getShard( void )
{
	return auth_info.servers[gSelectedDbServer].name;
}

bool userHasVIPStatus( void )
{
	return db_info.vip || AccountIsVIP(inventoryClient_GetAcctInventorySet(), inventoryClient_GetAcctStatusFlags());
}

bool userHasLL5Status( void )
{
	return ( accountLoyaltyRewardHasLevel( inventoryClient_GetLoyaltyPointsEarned(), "Level5" ) != 0 );
}

tWebStoreAuthType getStoreAuthType( void )
{
	if ( s_PlaySpanDigestTimeStamp != 0 ) 
	{
		return kWebStoreAuth_TimeStampSHA1;
	}
	else
	{
		return kWebStoreAuth_NoTimeStampMD5;
	}
}

const char* getUrlForPlaySpanStorePage( tStoreFrontPageId pageId )
{
	const char* pageURL = NULL;
	const AccountStoreAccessInfo* info = accountCatalog_GetStoreAccessInfo();

	switch ( pageId ) {
		case kStoreFrontHomePageId:			pageURL = info->playSpanURL_Home;			break;
		case kStoreFrontCategoryViewPageId:	pageURL = info->playSpanURL_CategoryView;	break;
		case kStoreFrontItemViewPageId:		pageURL = info->playSpanURL_ItemView;		break;
		case kStoreFrontAddToCartPageId:	pageURL = info->playSpanURL_AddToCart;		break;
		case kStoreFrontShowShoppingCart:	pageURL = info->playSpanURL_ShowCart;		break;
		case kPlayNcManageAccountPageId:	pageURL = info->playSpanURL_ManageAccount;	break;
		case kPlayNcSupportPagePageId:		pageURL = info->playSpanURL_SupportPage;	break;
		case kPlayNcSupportPagePageIdDE:	pageURL = info->playSpanURL_SupportPageDE;	break;
		case kPlayNcSupportPagePageIdFR:	pageURL = info->playSpanURL_SupportPageFR;	break;
		case kPlayNcUpgradeToVIPPageId:		pageURL = info->playSpanURL_UpgradeToVIP;	break;
		case kCohNewFeaturesPageId:			pageURL = info->cohURL_NewFeatures;			break;
		case kCohNewFeaturesUpdatePageId:	pageURL = info->cohURL_NewFeaturesUpdate;	break;
		default:							assert(false);	/* bad page id */			break;
	}
	return pageURL;
}

const char* getPlaySpanDomain( void )
{
	return accountCatalog_GetStoreAccessInfo()->playSpanDomain;
}

const char* getPlaySpanCatalogId( void )
{
	return accountCatalog_GetStoreAccessInfo()->playSpanCatalog;
}

void PlaySpanStoreLauncher_SetDigest(U32 timeStamp, const char *digest)
{
	s_PlaySpanDigestTimeStamp = timeStamp;
	memset(s_PlaySpanDigest, 0, sizeof(s_PlaySpanDigest));
	s_PlaySpanDigestLength = strlen(digest);
	cryptStore(s_PlaySpanDigest, digest, s_PlaySpanDigestLength);	
}

// this function can be called periodically to handle timeout of a
// HeroAuth request from PlayNC
void PlaySpanStoreLauncher_ServiceHeroAuths( void )
{
	int challengeSlot;
	tChallenge* mruChallenge;

	int mruID = s_nextChallengeID - 1;	// most recently used challenge ID

	// is there a current valid challenge to check?
	if ( mruID < 0 )
		return;			// never been a challenge

	challengeSlot = GetChallengeRequestSlot( mruID );
	mruChallenge = &s_ChallengedURLRequests[challengeSlot];

	if ( !mruChallenge->request_active || mruChallenge->request_key != mruID )
		return;	// most recently requested challenge has been serviced or cleared

	if (timerSecondsSince2000() - mruChallenge->request_time > kHeroAuthTimoutSeconds )
	{
		const char response_error[] = "timeout";

		// we should have the original challenge string to supply back
		// for matching purposes
		if ( eaSize(&mruChallenge->challengeParams) )
		{
			const char* challenge =  mruChallenge->challengeParams[0]->value;
			bool is_auth_complete = webBrowser_authorize( auth_info.name, response_error, challenge );
		}
		// clear the current challenge slot since it is serviced
		clearChallengeRequestSlot(challengeSlot);

		if (cmdAccessLevel()>ACCESS_USER) printf(" onHeroAuth: *failure* Authorization timeout. Server is not responding.\n");
	}
}

void PlaySpanStoreLauncher_ReceiveAuthKeyResponse( int request_key, const char* auth_key )
{
	int challengeSlot;
	
	if ( request_key < 0 ) {
		return;
	}
	
	challengeSlot = GetChallengeRequestSlot( request_key );
	if (( s_ChallengedURLRequests[challengeSlot].request_key != request_key ) ||
		( s_ChallengedURLRequests[challengeSlot].url == NULL )) {
		s_ChallengedURLRequests[challengeSlot].request_active = false;
		return;
	}

	// If this isn't the most recent challenge then just eat it
	if (request_key+1 != s_nextChallengeID)
	{
		s_ChallengedURLRequests[challengeSlot].request_active = false;
		return;	// previous requests are just abandoned
	}

	// we should have the original challenge string to supply back
	// for matching purposes
	if ( eaSize(&s_ChallengedURLRequests[challengeSlot].challengeParams) )
	{
		const char* challenge = s_ChallengedURLRequests[challengeSlot].challengeParams[0]->value;
		bool is_auth_complete = webBrowser_authorize( auth_info.name, auth_key, challenge );
		if (cmdAccessLevel()>ACCESS_USER) printf(" onHeroAuth: Authorization response received. Retrying request...\n");
	}
	
	// clear the current challenge slot since it is serviced
	clearChallengeRequestSlot(challengeSlot);
}

void clearChallengeRequestSlot( int challengeSlot )
{
	int i;
	tChallenge* pChallenge;

	assert( challengeSlot >= 0 );
	assert( challengeSlot < MAX_PENDING_CHALLENGED_URLS );
	pChallenge = ( s_ChallengedURLRequests + challengeSlot );

	pChallenge->request_active = false;
	pChallenge->request_key = -1;
	
	free( pChallenge->url );
	pChallenge->url = NULL;
	
	for ( i=0; i < eaSize(&pChallenge->challengeParams); i++ ) {
		free( (char*)pChallenge->challengeParams[i]->valName );
		free( (char*)pChallenge->challengeParams[i]->value );
		free( pChallenge->challengeParams[i] );
	}
	eaDestroy( &pChallenge->challengeParams );
}

void storeChallengeRequestInSlot( int challengeSlot, int request_key, 
									const char* URL, 
									const tPlaySpanStoreLauncherNameValuePair* digestData[], 
									int numDataItems )
{
	//
	//	Info is stored in a static array while the request is being
	//	processed by the AccountServer.
	//

	int i;
	tChallenge* pChallenge;
	assert( challengeSlot >= 0 );
	assert( challengeSlot < MAX_PENDING_CHALLENGED_URLS );
	pChallenge = ( s_ChallengedURLRequests + challengeSlot );
	
	clearChallengeRequestSlot( challengeSlot );

	pChallenge->request_active = true;
	pChallenge->request_key = request_key;
	
	pChallenge->url = strdup( URL );
	pChallenge->request_time = timerSecondsSince2000(); // to timeout the auth, in case AccountServer never responds

	eaCreate( &pChallenge->challengeParams );
	for ( i=0; i < numDataItems; i++ ) {
		tPlaySpanStoreLauncherNameValuePair* pPair = 
			(tPlaySpanStoreLauncherNameValuePair*)malloc( sizeof(tPlaySpanStoreLauncherNameValuePair) );
		pPair->valName = strdup( digestData[i]->valName );
		pPair->value   = strdup( digestData[i]->value );
		eaPush( &pChallenge->challengeParams, pPair );
	}
}

void requestPlayNCAuthKey( const char* URL, 
							const tPlaySpanStoreLauncherNameValuePair* digestData[], 
							int numDataItems )
{
	int i;
	char* digestBuffer = NULL;
	estrCreate( &digestBuffer );

	//	
	// For sending to the server, stick everything into one string.
	//
	for ( i=0; i < numDataItems; i++ ) {
		estrConcatCharString( &digestBuffer, digestData[i]->value );
	}
	
	//
	// Store this info to make it available when the response comes back.
	//
	storeChallengeRequestInSlot( 
					GetChallengeRequestSlot(s_nextChallengeID), s_nextChallengeID,
					URL, digestData, numDataItems ); 
	
	// Try sending the packet directly to the dbserver; otherwise, to the mapserver.
	if ( ! dbGetPlayNCAuthKey( s_nextChallengeID, auth_info.name, digestBuffer ) )
	{
		uiNetGetPlayNCAuthKey( s_nextChallengeID, auth_info.name, digestBuffer );
	}
	
	estrDestroy( &digestBuffer );
	s_nextChallengeID++;
}

// PlayNC http "HeroAuth" digest authorization
void PlaySpanStoreLauncher_RequestAuthorizationPlayNC( const char* url, const char* challenge )
{
	tPlaySpanStoreLauncherNameValuePair pair;
	tPlaySpanStoreLauncherNameValuePair* pairs[1] = {&pair};
	pair.valName = "challenge";
	pair.value = challenge;
	requestPlayNCAuthKey( url, pairs, 1 );
}

void getPlaySpanDigest(char *digest, size_t digest_size)
{
	devassert(s_PlaySpanDigestLength < digest_size);
	cryptRetrieve(digest, s_PlaySpanDigest, s_PlaySpanDigestLength);
	digest[s_PlaySpanDigestLength] = 0;
}

bool gotoPlaySpanStorePage( 
				tStoreFrontPageId pageId,
				int numDataVals,
				tPlaySpanStoreLauncherNameValuePair* dataVals,
				const char* tail)
{
	char storeURL[2048];
	if ( ! buildStoreURL( 
				pageId,
				numDataVals,
				dataVals,
				tail,
				storeURL,
				ARRAY_SIZE(storeURL) ) ) {
		// Might happen if we haven't gotten the URL specs from the AccountServer yet.
		return false;
	}
	
	return uiWebBrowser_goto_url( storeURL );
}

void addToURL( char** anEStrBuffer, int* pNumArgsAdded, const char* argName, const char* argValue )
{
	estrConcatf(anEStrBuffer, "%s%s=%s", *pNumArgsAdded ? "&" : "?", argName, argValue);
	(*pNumArgsAdded)++;
}

bool addFragmentToURL( char** anEStrBuffer, 
				const char* fragment )
{
	if (( fragment == NULL ) || ( *fragment == '\0' )) {
		// If argValue is NULL or empty, we do nothing (not an error).
		// So the caller doesn't need to check whether the value
		// it is sending is valid or not.
		return true;
	}
	estrConcatf(anEStrBuffer, "#%s", fragment);
	return true;
}

bool buildStoreURL( 
				tStoreFrontPageId pageId,
				int numDataVals,
				tPlaySpanStoreLauncherNameValuePair* dataVals, 
				const char* tail,
				char* urlBuffer,
				size_t urlBuffSz )
{
	bool success				= true;
	bool addLoginArgs			= false;
	int numArgsAdded			= 0;
	char* pageUrl				= NULL;
	char* fragment				= NULL;
	char* dottedAuth			= NULL;
	const char* p				= NULL;
	size_t urlLen;
	char* storeURLWorkStr		= NULL;
	int i;
	tWebStoreAuthType storeAuthType = getStoreAuthType();
	
	urlBuffer[0] = '\0';
	
	if ( ! PlaySpanStoreLauncher_IsLoaded() ) {
		return false;
	}
	
	//
	//	- Separate the main URL string from the "#string" 'fragment.
	//	- Make the base + page URL
	//
	p = getUrlForPlaySpanStorePage( pageId );
	if (( p == NULL ) || ( *p == '\0' )) {
		return false;
	}

	estrCreate( &storeURLWorkStr );
	
	urlLen		= strlen( p );
	pageUrl		= (char*)_alloca( urlLen + 1 );
	strcpy_s( pageUrl, urlLen+1, p );
	
	fragment	= strchr( pageUrl, '#' );
	if ( fragment != NULL ) {
		*fragment = '\0';	// null terminate 'baseUrl'
		fragment++;			// fragment starts after the '#'
	}
	if ( *pageUrl == '/' ) {
		// Relative path within PlaySpan domain
		estrConcatCharString( &storeURLWorkStr, getPlaySpanDomain() );
		addLoginArgs = true;
	}
	estrConcatCharString( &storeURLWorkStr, pageUrl );
		
	if ( addLoginArgs ) {
		char playSpanDigest[PLAYSPAN_DIGEST_MAX];
		
		//
		//	Make a 'dotted' account name, using the current
		//	AccountServer's name as the prefix.
		//
		estrCreate(&dottedAuth);
		estrConcatf(&dottedAuth, "%s.%u", accountCatalog_GetMtxEnvironment(), auth_info.uid);

		//
		//	Standard params
		//	
		addToURL( &storeURLWorkStr, &numArgsAdded, "userid", dottedAuth );
		if ( storeAuthType == kWebStoreAuth_NoTimeStampMD5 )
		{
			addToURL( &storeURLWorkStr, &numArgsAdded, "sn",	getPlaySpanCatalogId() );
		}
	
		{
			const char* localeStr = getLocale();
			if ( *SafeStringPtr(localeStr) != '\0' ) {
				addToURL( &storeURLWorkStr, &numArgsAdded, "lang", localeStr );
			}
		}
		{
			const char* charLevel = getCharLevel();
			if ( *SafeStringPtr(charLevel) != '\0' ) {
				addToURL( &storeURLWorkStr, &numArgsAdded, "r.charlevel", charLevel );
			}
		}
		{
			const char* archetypeStr = getCharArchetype();
			if ( *SafeStringPtr(archetypeStr) != '\0' ) {
				addToURL( &storeURLWorkStr, &numArgsAdded, "r.archetype", archetypeStr );
			}
		}

		if ( userHasVIPStatus() ) {
			addToURL( &storeURLWorkStr, &numArgsAdded, "r.vip", "1" );
		}

		if ( userHasLL5Status() ) {
			addToURL( &storeURLWorkStr, &numArgsAdded, "r.ll5", "1" );
		}
		
		if ( storeAuthType == kWebStoreAuth_NoTimeStampMD5 )
		{
			getPlaySpanDigest(playSpanDigest, sizeof(playSpanDigest));
			addToURL( &storeURLWorkStr, &numArgsAdded, "digest", playSpanDigest );
		}
		else
		{
			char timeStampBuffer[80];
			// PlaySpan wants the time x 1000, so we stuff three zeroes on the end,
			sprintf_s( timeStampBuffer, ARRAY_SIZE(timeStampBuffer),"%u000", s_PlaySpanDigestTimeStamp );
			addToURL( &storeURLWorkStr, &numArgsAdded, "timestamp", timeStampBuffer );
			addToURL( &storeURLWorkStr, &numArgsAdded, "partneraccessid",	"NCSF" );
			getPlaySpanDigest(playSpanDigest, sizeof(playSpanDigest));
			addToURL( &storeURLWorkStr, &numArgsAdded, "signature", playSpanDigest );
		}
		memset(playSpanDigest, 0, sizeof(playSpanDigest));	

	}
	
	//
	//	Custom params sent by the caller
	//
	for ( i=0; ( success ) && ( i < numDataVals ); i++ ) {
		// Skip NULL and "" values.
		if ( *SafeStringPtr( dataVals[i].value ) != '\0' ) {
			addToURL( &storeURLWorkStr, &numArgsAdded, dataVals[i].valName,	dataVals[i].value );
		}
	}
	
	addFragmentToURL( &storeURLWorkStr, fragment );
	estrConcatCharString( &storeURLWorkStr, tail );
	
	urlLen = strlen( storeURLWorkStr );
	if ( estrLength( &storeURLWorkStr ) < urlBuffSz ) {
		strcpy_s( urlBuffer, urlBuffSz, storeURLWorkStr );
	} else {
		urlBuffer[0] = '\0';
		success = false;
	}
	
	estrDestroy( &dottedAuth );
	estrDestroy( &storeURLWorkStr );
	
	return success;
}
