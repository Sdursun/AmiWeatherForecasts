/* 
AmiWeatherForecasts by emarti, Murat Ozdemir 
V1.1 Oct 2023
V1.0 Feb 2022 (not published)
*/

#include "includes.h"
#include <weather.h>
#include <httpgetlib.h>
#include "version.h"
#include "images.h"
#include <funcs.h>

#define USERSTARTUP "SYS:S/User-Startup"

char location[BUFFER/8];
char unit[10];
char lang[5];
char apikey[BUFFER/16];
char days[BUFFER/16];
char months[BUFFER/16];
char displayIcon[2];
char displayLocation[2];
char displayDescription[2];
char displayDateTime[2];
int addX, addY, addH;

char **amonths;
char **adays;
    
struct GfxBase       *GfxBase;
struct IntuitionBase *IntuitionBase;
struct Window        *Window;
struct RastPort      *RP;
struct Screen        *screen;
struct Library       *CheckBoxBase      = NULL;
struct VisualInfo*    VisualInfoPtr     = NULL;
struct Menu*          amiMenu           = NULL;

struct NewWindow NewWindow =
{
    0x00, 0x00,
    1, 0x0A,
    0,1,
    IDCMP_VANILLAKEY|
    IDCMP_MOUSEBUTTONS|
    IDCMP_MENUPICK,
    WFLG_BORDERLESS|
    WFLG_NEWLOOKMENUS|
    WFLG_WBENCHWINDOW,
    NULL,NULL,NULL,NULL,NULL,
    0,0,0,0,
    WBENCHSCREEN
};

char txt[BUFFER/4];
int indx;
int newWidth;
int barHeight;
char** wdata;    

char wf[50] = "RAM:weather.json";
char weatherText[BUFFER] = "";
char weatherURL[BUFFER/2] = ""; 
int refresh;

void write(char *text);
void update(void);
void beforeClosing(void);

char appPath[PATH_MAX+NAME_MAX];
char curPath[PATH_MAX];
char manualPath[PATH_MAX+NAME_MAX];

#define PREFFILEPATH	"SYS:Prefs/Env-Archive/AmiWeatherForecasts.prefs"
void SavePrefs(void);
void LoadPrefs(void);
void createPreferencesWin(void);
void loadDefaults(void);

BOOL IsAddedToUserStartup(void);
void RemoveLineFromUserStartup(void);

#define CLANGCOUNT  47
STRPTR clanguages[] = {
        "af",// Afrikaans
        "al",// Albanian
        "ar",// Arabic
        "az",// Azerbaijani
        "bg",// Bulgarian
        "ca",// Catalan
        "cz",// Czech
        "da",// Danish
        "de",// German
        "el",// Greek
        "en",// English
        "eu",// Basque
        "fa",// Persian (Farsi)
        "fi",// Finnish
        "fr",// French
        "gl",// Galician
        "he",// Hebrew
        "hi",// Hindi
        "hr",// Croatian
        "hu",// Hungarian
        "id",// Indonesian
        "it",// Italian
        "ja",// Japanese
        "kr",// Korean
        "la",// Latvian
        "lt",// Lithuanian
        "mk",// Macedonian
        "no",// Norwegian
        "nl",// Dutch
        "pl",// Polish
        "pt",// Portuguese
        "pt_br",// Português Brasil
        "ro",// Romanian
        "ru",// Russian
        "sv",// se",//	Swedish
        "sk",// Slovak
        "sl",// Slovenian
        "sp",// es",//	Spanish
        "sr",// Serbian
        "th",// Thai
        "tr",// Turkish
        "ua",// uk",// Ukrainian
        "vi",// Vietnamese
        "zh_cn",// Chinese Simplified
        "zh_tw",// Chinese Traditional
        "zu",// Zulu
        NULL};

ULONG getIndexChooser(STRPTR val);

struct NewMenu amiMenuNewMenu[] =
{
    NM_TITLE, (STRPTR)"AmiWeatherForecasts"         ,  NULL , 0, NULL, (APTR)~0,
    NM_ITEM , (STRPTR)"About"               		,  "A" , 0, 0L, (APTR)~0,
    NM_ITEM , NM_BARLABEL                          	,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Quit"                      	,  "Q" , 0, 0L, (APTR)~0,
	
    NM_TITLE, (STRPTR)"Tools"              		    ,  NULL , 0, NULL, (APTR)~0,
	NM_ITEM , (STRPTR)"Update"                 	    ,  "U" , 0, 0L, (APTR)~0,

    NM_TITLE, (STRPTR)"Settings"                    ,  NULL , 0, NULL, (APTR)~0,
    NM_ITEM , (STRPTR)"Preferences"                 ,  "P" , 0, 0L, (APTR)~0,
    NM_ITEM , NM_BARLABEL                          	,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Add to User-Startup"         ,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , NM_BARLABEL                           ,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Remove from User-Startup"    ,  NULL , 0, 0L, (APTR)~0,
    
    NM_TITLE, (STRPTR)"Help"                        ,  NULL , 0, NULL, (APTR)~0,
    NM_ITEM , (STRPTR)"Manual"                      ,  "M" , 0, 0L, (APTR)~0,
	
    NM_END  , NULL                                  ,  NULL , 0, 0L, (APTR)~0
};

ULONG amiMenuTags[] =
{
    (GT_TagBase+67), TRUE,
    (TAG_DONE)
};

int makeMenu(APTR MenuVisualInfo)
{
    if (NULL == (amiMenu = CreateMenusA( amiMenuNewMenu, NULL))) return( 1L );
    LayoutMenusA( amiMenu, MenuVisualInfo, (struct TagItem *)(&amiMenuTags[0]));
    return( 0L );
}

struct EasyStruct aboutReq =
{
    sizeof(struct EasyStruct),
    0,
    "About "APPNAME,
    ABOUT,
    "Ok"
};

struct EasyStruct addedUserStartup =
{
    sizeof(struct EasyStruct),
    0,
    "Info - "APPNAME,
    "Each time your Amiga is booted, this application is executed.",
    "Ok"
};

struct EasyStruct addedUserStartupPreviously =
{
    sizeof(struct EasyStruct),
    0,
    "Warning - "APPNAME,
    "It was previously added command line to the User-Startup file\nto execute this application when startup your Amiga.",
    "Ok"
};

struct EasyStruct RemoveFromUserStartup =
{
    sizeof(struct EasyStruct),
    0,
    "Info - "APPNAME,
    "It was removed command line from the User-Startup file\nto execute this application when startup your Amiga.",
    "Ok"
};

struct EasyStruct RemoveFromUserStartupW =
{
    sizeof(struct EasyStruct),
    0,
    "Warning - "APPNAME,
    "The User-Startup file not include command line that execute this application.",
    "Ok"
};

void loadDefaults(void)
{
    strcpy(location, "istanbul,tr");
    strcpy(unit,"metric");
    strcpy(lang,"en");
    strcpy(apikey,"e45eeea40d4a9754ef176588dd068f18");
    strcpy(months,"Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec");
    strcpy(days,"Sun,Mon,Tue,Wed,Thu,Fri,Sat");
    strcpy(displayIcon,"1");
    strcpy(displayLocation,"1");
    strcpy(displayDescription,"1");
    strcpy(displayDateTime,"1");
    addX = addY = addH = 0;
}
    
int main(int argc, char **argv)
{
    register BOOL loop = TRUE;
    register unsigned short curMin=99;
    register struct tm *dt;
    register struct IntuiMessage *msg;
    register char screenText[BUFFER/2];
    char strdatetime[0xFF];

    // Pref file
    if(!fileExist(PREFFILEPATH))
	{
        loadDefaults();
		SavePrefs();
	}
		
    GetCurrentDirName(curPath, PATH_MAX);
	strcpy(appPath, curPath);
	strcat(appPath, "/");
    strcat(appPath, APPNAME);
	
	strcpy(manualPath, "SYS:Utilities/MultiView ");
	strcat(manualPath, appPath);
	strcat(manualPath, ".guide");

    
    wdata = (char**)malloc(4*sizeof(char*));
    adays = (char**)malloc(7*sizeof(char*));
    amonths = (char**)malloc(12*sizeof(char*));
    
    LoadPrefs();

    if ((IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",0L))==0)
        return SORRY;

    if ((GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",0L))==0)
    {
        CloseLibrary(IntuitionBase);
        return SORRY;
    }

    if ((CheckBoxBase =(struct CheckBoxBase *) OpenLibrary ("gadgets/checkbox.gadget",0L))==0)
    {
        CloseLibrary(IntuitionBase);
        CloseLibrary(GfxBase);
        return SORRY;
    }

    screen = LockPubScreen(NULL);

    if ((screen==NULL) || ((Window=(struct Window*) OpenWindow(&NewWindow))==0L))
    {
        UnlockPubScreen(NULL, screen);
        CloseLibrary(IntuitionBase);
        CloseLibrary(GfxBase);
        CloseLibrary(CheckBoxBase);
        return SORRY;
    }
 
    RP=(struct RastPort*)Window->RPort;

    // get weather URL
    strcpy(weatherURL, getURL(apikey, location, unit, lang));
    // get weather info
    update();

    barHeight = screen->BarHeight+1;// screen->WBorTop + screen->Font->ta_YSize;

    VisualInfoPtr = GetVisualInfo(screen, NULL);

    makeMenu(VisualInfoPtr);
    SetMenuStrip(Window, amiMenu);

    while(loop)
    {
        time_t t = time(NULL);
        dt=localtime(&t);
        if ((curMin!=dt->tm_min) || (refresh < 5))
        {
            curMin=dt->tm_min;//sec; 
            
            sprintf(strdatetime, " %s %02d, %s %02d:%02d ", amonths[dt->tm_mon], dt->tm_mday, adays[dt->tm_wday], dt->tm_hour, dt->tm_min);
            sprintf(screenText, " %s ", wdata[2]);
            if(!STREQUAL(displayLocation, "0")) { strcat(screenText, wdata[1]); strcat(screenText, " ");}
            if(!STREQUAL(displayDescription, "0")) { strcat(screenText, "["); strcat(screenText, wdata[0]); strcat(screenText, "] ");}
            if(!STREQUAL(displayDateTime, "0")) { strcat(screenText, strdatetime);}

            
            newWidth = 2 + strlen(screenText);
            ChangeWindowBox(Window, IntuitionBase->ActiveScreen->Width-( (newWidth+4)*RP->TxWidth ) + addX, -1 + addY, newWidth*RP->TxWidth, barHeight + addH);
            write(screenText);
            refresh++;
            
            // update periodically
            if((curMin%PERIOD4UPDATE==0)&&(refresh>5)) update();
            
        } 
         
        while ((msg = (struct IntuiMessage*)GetMsg( Window->UserPort)))
        {
            UWORD code = msg->Code;
            char cmd[512];

            switch (msg->Class)
            {
            case IDCMP_VANILLAKEY:
                switch(msg->Code)
                {
                    case 0x1B:                          // press ESC to exit
                        loop=FALSE;
                        break;
                    default:
                        break;
                }
                break;

            case IDCMP_MENUPICK: 
                while (code != MENUNULL) { 
                    struct MenuItem *item = ItemAddress(amiMenu, code); 
                    struct IntuiText *text = item->ItemFill;
                    if STREQUAL(text->IText, "About") EasyRequest(Window, &aboutReq, NULL, NULL);
                    else if STREQUAL(text->IText, "Quit") 
                                loop=FALSE;
                    else if STREQUAL(text->IText, "Update") 
                                update();
                    else if STREQUAL(text->IText, "Manual") 
                                Execute(manualPath, NULL, NULL);
                    else if STREQUAL(text->IText, "Preferences") 
                                createPreferencesWin();
                    else if STREQUAL(text->IText, "Add to User-Startup")
                            {
                                if (IsAddedToUserStartup())
                                {
                                    EasyRequest(Window, &addedUserStartupPreviously, NULL, NULL);
                                }
                                else
                                {
                                    sprintf(cmd, "echo \"*N*NRun >NIL: %s ; %s\" >> %s >NIL:", appPath, APPNAME, USERSTARTUP);      
                                    Execute(cmd, NULL, NULL);
                                    EasyRequest(Window, &addedUserStartup, NULL, NULL);
                                }
                            }
                    else if STREQUAL(text->IText, "Remove from User-Startup")
                            {
                                if(IsAddedToUserStartup())
                                {
                                    RemoveLineFromUserStartup();
                                    EasyRequest(Window, &RemoveFromUserStartup, NULL, NULL);
                                }
                                else
                                {
                                    EasyRequest(Window, &RemoveFromUserStartupW, NULL, NULL);
                                }
                            }
                                
                    code = item->NextSelect;
                }
                break;
            
            default:
                break;
            }

            ReplyMsg(msg);
        }
            
        Delay(WAITONESECOND/4);
        
    }

    beforeClosing();
    return 0x00;
}

void write(char *text)
{
    int i,ii;
    SetAPen(RP,0x02);
    Move(RP,0,0);
    RectFill(RP,0,0,newWidth*RP->TxWidth,barHeight);  
    SetAPen(RP,0x01);
    SetBPen(RP,0x02);
    
    // write text
    Move(RP,20, 8 + (barHeight-screen->Font->ta_YSize )/2);
    Text(RP,text, newWidth -2);
    
    // for testing => indx=?;
    // draw weather icon
    if(!STREQUAL(displayIcon, "0"))
    {
        for(i=10*indx;i<10*indx+10;i++)
        {
            for(ii=0;ii<24;ii++)
            {
                switch(images[i][ii])
                {
                    case '*': SetAPen(RP, 1); break;
                    case '+': SetAPen(RP, 3); break;
                    case '-': SetAPen(RP, 0); break;
                    case ' ': SetAPen(RP, 2); break;
                }
                WritePixel(RP, ii, i%10 + (barHeight-screen->Font->ta_YSize )/2);
            }
        } 
    }
}

void update(void)
{
    httpget(weatherURL, wf);
    
    strcpy(weatherText, getWeatherData(wf));  
    strcpy(weatherText, convertTRChar(weatherText));     
    
    wdata = getArray(weatherText, "|", 4);
    
    indx = iconIndex(wdata[3]);
    strcpy(wdata[2], temperatureWithUnit(wdata[2], unit));
    
    if (fileExist(wf))
    {
        Execute("delete RAM:weather.json >NIL:",NULL,NULL);
    }
    
    refresh = 0;
}

void beforeClosing()
{
    UnlockPubScreen(NULL, screen);
    ClearMenuStrip(Window);
    FreeVisualInfo(VisualInfoPtr);
    CloseWindow(Window);
    CloseLibrary(IntuitionBase);
    CloseLibrary(GfxBase);
    CloseLibrary(CheckBoxBase);
}

/*
### SYS:Prefs/Env-Archive/AmiWeatherForecasts.prefs ###
*/
BPTR fp;
char fpStr[BUFFER/2];

// load settings from pref file
void LoadPrefs(void)
{
    char tm[64], td[64];
	fp = Open(PREFFILEPATH, MODE_OLDFILE);
	if (fp)
	{
        UBYTE buffer[BUFFER/8];

        FGets(fp, buffer, BUFFER/8);
        sprintf(location, "%s", buffer);
        location[strlen(location)-1] = '\0';    // location

        FGets(fp, buffer, BUFFER/8);
        sprintf(unit, "%s", buffer);
        unit[strlen(unit)-1] = '\0';    // unit

        FGets(fp, buffer, BUFFER/8);
        sprintf(lang, "%s", buffer);
        lang[strlen(lang)-1] = '\0';    // lang

        FGets(fp, buffer, BUFFER/8);
        sprintf(apikey, "%s", buffer);
        apikey[strlen(apikey)-1] = '\0';    // apikey

        FGets(fp, buffer, BUFFER/8);
        sprintf(days, "%s", buffer);
        days[strlen(days)-1] = '\0';    // days

        FGets(fp, buffer, BUFFER/8);
        sprintf(months, "%s", buffer);
        months[strlen(months)-1] = '\0';    // months

        FGets(fp, buffer, BUFFER/8);
        sprintf(displayIcon, "%s", buffer);
        displayIcon[strlen(displayIcon)-1] = '\0';    // displayIcon

        FGets(fp, buffer, BUFFER/8);
        sprintf(displayLocation, "%s", buffer);
        displayLocation[strlen(displayLocation)-1] = '\0';    // displayLocation

        FGets(fp, buffer, BUFFER/8);
        sprintf(displayDescription, "%s", buffer);
        displayDescription[strlen(displayDescription)-1] = '\0';    // displayDescription

        FGets(fp, buffer, BUFFER/8);
        sprintf(displayDateTime, "%s", buffer);
        displayDateTime[strlen(displayDateTime)-1] = '\0';    // displayDateTime

        FGets(fp, buffer, BUFFER/8);
        addX = atoi(buffer);    // addX

        FGets(fp, buffer, BUFFER/8);
        addY = atoi(buffer);    // addY

        FGets(fp, buffer, BUFFER/8);
        addH = atoi(buffer);    // addH

        strcpy(tm, months);
        strcpy(td, days);
        amonths= getArray(tm, ",", 12);
        adays = getArray(td, ",", 7);
        
		Close(fp);
	}
}

// save settings
void SavePrefs(void)
{
	fp = Open(PREFFILEPATH, MODE_NEWFILE);
	if (fp)
	{
		sprintf(fpStr, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%d\n%d\n%d\n", location, unit, lang, apikey, days, months, displayIcon, displayLocation, displayDescription, displayDateTime, addX, addY, addH);
		FPuts(fp, fpStr);
		Close(fp);
	}
}

ULONG getIndexChooser(STRPTR val)
{
    ULONG i;
    for ( i = 0; i < CLANGCOUNT; i++)
    {
        if(STREQUAL(clanguages[i],val)) break;
    }
    return i;
}

// Preferences window
void createPreferencesWin(void)
{
	enum {
		IDFIRST=0x00,
		// gadget id starts
		IDSTRINGLOCATION,
        IDSTRINGLANGUAGE,
        IDINTEGERUNIT,
        IDSTRINGDAYS,
        IDSTRINGMONTHS,
        IDSTRINGAPIKEY,		
        IDCHECKBOXDISPLAYICON,
        IDCHECKBOXDISPLAYLOCATION,
        IDCHECKBOXDISPLAYDESCRIPTION,
        IDCHECKBOXDISPLAYDATETIME,		
        IDINTEGERADDX,
        IDINTEGERADDY,
        IDINTEGERADDH,
		IDBCANCEL,
        IDBSAVEANDCLOSE,
		// end of gadget id
		IDLAST
	};
	
	struct Gadget *co[IDLAST-1];
	struct Gadget *vcreatePreferencesParent = NULL;
	Object *createPreferencesWinObj = NULL;
	struct Window *createPreferencesWin = NULL;
	ULONG signalcreatePreferences, resultcreatePreferences;
	signed char donecreatePreferences;
	UWORD codecreatePreferences;

    ULONG dIcon, dDescription, dLocation, dDatetime;
	
	struct List *chooserUnit;
	chooserUnit = ChooserLabels( "metric","imperial", NULL );

    dIcon = STREQUAL(displayIcon, "1")?TRUE:FALSE;
    dLocation = STREQUAL(displayLocation, "1")?TRUE:FALSE;
    dDescription = STREQUAL(displayDescription, "1")?TRUE:FALSE;
    dDatetime = STREQUAL(displayDateTime, "1")?TRUE:FALSE;
	
	if (!(createPreferencesWinObj = NewObject
    (   WINDOW_GetClass(),
        NULL,
		WA_PubScreen,             screen,
        WA_Title,           	  "Preferences - AmiWeatherForecasts",
        WA_Activate,              TRUE,
        WA_DragBar,               TRUE,
        WA_CloseGadget,           TRUE,
		WA_SizeGadget, 			  TRUE,
        WA_SmartRefresh,          TRUE,
        WA_MinWidth,              512,
        WA_MinHeight,             30,
        WA_MaxWidth,              512,
        WA_MaxHeight,             -1,
        WA_InnerWidth,                 512,
        WA_InnerHeight,                30,
        WA_Left,                  0,
        WA_Top,                   0,
        WA_SizeBRight,            FALSE,
        WA_SizeBBottom,           TRUE,
        WA_NewLookMenus,          TRUE,
        WINDOW_Position,          WPOS_CENTERSCREEN,
		WINDOW_ParentGroup,       vcreatePreferencesParent = VGroupObject,
												LAYOUT_SpaceOuter, TRUE,
												LAYOUT_DeferLayout, TRUE,

                                                // Text 
            									LAYOUT_AddChild, HGroupObject,
                                                                LAYOUT_AddChild,  LabelObject,
                                                                                LABEL_Justification, 0,
                                                                                LABEL_Text, "               ",
                                                                End,
                                                                CHILD_Label,  LabelObject,
                                                                                LABEL_Justification, 0,
                                                                                LABEL_Text, " Read guide [Help/Manual menu] for more details.",
                                                                End,
                                                End,

												// LOCATION
                                                LAYOUT_AddChild, co[0] = (struct Gadget *) StringObject,
                                                                GA_ID, IDSTRINGLOCATION,
                                                                GA_RelVerify, TRUE,
                                                                STRINGA_TextVal, location,
                                                                STRINGA_MaxChars, 64,
                                                                STRINGA_Justification, GACT_STRINGLEFT,
                                                                STRINGA_HookType, SHK_CUSTOM,
                                                End,
                                                CHILD_Label,  LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "      Location ",
                                                End,

                                                // UNIT
                                                LAYOUT_AddChild, co[1] = (struct Gadget *) ChooserObject,
                                                                GA_ID, IDINTEGERUNIT,
                                                                GA_RelVerify, TRUE,
                                                                CHOOSER_PopUp, TRUE,
                                                                CHOOSER_MaxLabels, 2,
                                                                CHOOSER_Offset, 0,
                                                                CHOOSER_Selected, (STREQUAL(unit,"metric"))?0:1,
                                                                CHOOSER_Labels, chooserUnit,
                                                End,                                                
                                                CHILD_Label, LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "          Unit ",
                                                End,

                                                // LANGUAGE
                                                LAYOUT_AddChild, co[2] = (struct Gadget *) ChooserObject,
                                                                GA_ID, IDINTEGERUNIT,
                                                                GA_RelVerify, TRUE,
                                                                CHOOSER_PopUp, TRUE,
                                                                CHOOSER_MaxLabels, 46,
                                                                CHOOSER_Offset, 0,
                                                                CHOOSER_Selected, getIndexChooser(lang),
                                                                CHOOSER_LabelArray, clanguages,
                                                End,  
                                                
                                                CHILD_Label,  LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "      Language ",
                                                End,

                                                // DAYS
                                                LAYOUT_AddChild, co[3] = (struct Gadget *) StringObject,
                                                                GA_ID, IDSTRINGDAYS,
                                                                GA_RelVerify, TRUE,
                                                                STRINGA_TextVal, days,
                                                                STRINGA_MaxChars, 32,
                                                                STRINGA_Justification, GACT_STRINGLEFT,
                                                                STRINGA_HookType, SHK_CUSTOM,
                                                End,
                                                
                                                CHILD_Label,  LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "          Days ",
                                                End,

                                                // MONTHS
                                                LAYOUT_AddChild, co[4] = (struct Gadget *) StringObject,
                                                                GA_ID, IDSTRINGMONTHS,
                                                                GA_RelVerify, TRUE,
                                                                STRINGA_TextVal, months,
                                                                STRINGA_MaxChars, 64,
                                                                STRINGA_Justification, GACT_STRINGLEFT,
                                                                STRINGA_HookType, SHK_CUSTOM,
                                                End,
                                                
                                                CHILD_Label,  LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "        Months ",
                                                End,

                                                // APIKEY
                                                LAYOUT_AddChild, co[5] = (struct Gadget *) StringObject,
                                                                GA_ID, IDSTRINGAPIKEY,
                                                                GA_RelVerify, TRUE,
                                                                STRINGA_TextVal, apikey,
                                                                STRINGA_MaxChars, 64,
                                                                STRINGA_Justification, GACT_STRINGLEFT,
                                                                STRINGA_HookType, SHK_CUSTOM,
                                                End,
                                                
                                                CHILD_Label,  LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "        APIKEY ",
                                                End,

                                                // Displays icon
                                                LAYOUT_AddChild, co[6] = (struct Gadget *) CheckBoxObject,
                                                                                GA_ID, IDCHECKBOXDISPLAYICON,
                                                                                GA_RelVerify, TRUE,
                                                                                GA_Selected, dIcon,
                                                                                GA_Text, "Displays icon",
                                                                                CHECKBOX_TextPen, 1,
                                                                                CHECKBOX_FillTextPen, 1,
                                                                                CHECKBOX_BackgroundPen, 0,
                                                                                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                                End,
                                                CHILD_Label,  LabelObject,
                                                            LABEL_Justification, 0,
                                                            LABEL_Text, "               ",
                                                End,

                                                // Displays location
                                                LAYOUT_AddChild, co[7] = (struct Gadget *) CheckBoxObject,
                                                                                GA_ID, IDCHECKBOXDISPLAYLOCATION,
                                                                                GA_RelVerify, TRUE,
                                                                                GA_Selected, dLocation,
                                                                                GA_Text, "Displays location",
                                                                                CHECKBOX_TextPen, 1,
                                                                                CHECKBOX_FillTextPen, 1,
                                                                                CHECKBOX_BackgroundPen, 0,
                                                                                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                                End,
                                                CHILD_Label,  LabelObject,
                                                            LABEL_Justification, 0,
                                                            LABEL_Text, "               ",
                                                End,
                                                        
                                                // Displays description
                                                LAYOUT_AddChild, co[8] = (struct Gadget *) CheckBoxObject,
                                                                                GA_ID, IDCHECKBOXDISPLAYDESCRIPTION,
                                                                                GA_RelVerify, TRUE,
                                                                                GA_Selected, dDescription,
                                                                                GA_Text, "Displays description",
                                                                                CHECKBOX_TextPen, 1,
                                                                                CHECKBOX_FillTextPen, 1,
                                                                                CHECKBOX_BackgroundPen, 0,
                                                                                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                                End,
                                                CHILD_Label,  LabelObject,
                                                            LABEL_Justification, 0,
                                                            LABEL_Text, "               ",
                                                End,

                                                // Displays date & time
                                                LAYOUT_AddChild, co[9] = (struct Gadget *) CheckBoxObject,
                                                                                GA_ID, IDCHECKBOXDISPLAYDATETIME,
                                                                                GA_RelVerify, TRUE,
                                                                                GA_Selected, dDatetime,
                                                                                GA_Text, "Displays date & time",
                                                                                CHECKBOX_TextPen, 1,
                                                                                CHECKBOX_FillTextPen, 1,
                                                                                CHECKBOX_BackgroundPen, 0,
                                                                                CHECKBOX_TextPlace, PLACETEXT_RIGHT,
                                                End,
                                                CHILD_Label,  LabelObject,
                                                            LABEL_Justification, 0,
                                                            LABEL_Text, "               ",
                                                End,

                                                // Add to pos X
                                                LAYOUT_AddChild, co[10] = (struct Gadget *) IntegerObject,
                                                                                GA_ID, IDINTEGERADDX,
                                                                                GA_RelVerify, TRUE,
                                                                                INTEGER_Number, addX,
                                                                                INTEGER_MaxChars, 2,
                                                                                INTEGER_Minimum, -4,
                                                                                INTEGER_Maximum, 4,
                                                                                INTEGER_Arrows, TRUE,
                                                                                STRINGA_Justification, GACT_STRINGRIGHT,
                                                End,
                                                CHILD_Label, LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "  Add to Pos X ",
                                                End,

                                                // Add to pos Y
                                                LAYOUT_AddChild, co[11] = (struct Gadget *) IntegerObject,
                                                                                GA_ID, IDINTEGERADDY,
                                                                                GA_RelVerify, TRUE,
                                                                                INTEGER_Number, addY,
                                                                                INTEGER_MaxChars, 2,
                                                                                INTEGER_Minimum, -4,
                                                                                INTEGER_Maximum, 4,
                                                                                INTEGER_Arrows, TRUE,
                                                                                STRINGA_Justification, GACT_STRINGRIGHT,
                                                End,
                                                CHILD_Label, LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "  Add to Pos Y ",
                                                End,

												// Add to Height
                                                LAYOUT_AddChild, co[12] = (struct Gadget *) IntegerObject,
                                                                                GA_ID, IDINTEGERADDH,
                                                                                GA_RelVerify, TRUE,
                                                                                INTEGER_Number, addH,
                                                                                INTEGER_MaxChars, 2,
                                                                                INTEGER_Minimum, -4,
                                                                                INTEGER_Maximum, 4,
                                                                                INTEGER_Arrows, TRUE,
                                                                                STRINGA_Justification, GACT_STRINGRIGHT,
                                                End,
                                                CHILD_Label, LabelObject,
                                                                LABEL_Justification, 0,
                                                                LABEL_Text, "  Add to Height ",
                                                End,

                                                // Buttons
												LAYOUT_AddChild, HGroupObject,
													
                                                    LAYOUT_AddChild, co[13] = ButtonObject,
															GA_Text, "_Cancel",
															GA_RelVerify, TRUE,
															GA_ID, IDBCANCEL,
                                                    End,

                                                    LAYOUT_AddChild, LabelObject,
													  LABEL_Justification, 0,
													  LABEL_Text, "     ",
												    End,

													LAYOUT_AddChild, co[14] = ButtonObject,
															GA_Text, "_Save & Close",
															GA_RelVerify, TRUE,
															GA_ID, IDBSAVEANDCLOSE,
                                                    End,
												End,

											End
	)))
	{
		donecreatePreferences = TRUE;
	}
		
	createPreferencesWin = RA_OpenWindow(createPreferencesWinObj);
	GetAttr(WINDOW_SigMask, createPreferencesWinObj, &signalcreatePreferences);	
	
	donecreatePreferences=FALSE;
	
	while(!donecreatePreferences)
    {   
		Wait(signalcreatePreferences | (1 << createPreferencesWin->UserPort->mp_SigBit));

        while ((resultcreatePreferences = DoMethod(createPreferencesWinObj, WM_HANDLEINPUT, &codecreatePreferences)) != WMHI_LASTMSG)
        {   
            STRPTR vLocation, vDays, vMonths, vApikey;
            ULONG iLang, iUnit;
            char tm[64], td[64];

            switch (resultcreatePreferences & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    donecreatePreferences = TRUE;
                break;
				
				case WMHI_GADGETUP:
                    switch(resultcreatePreferences & WMHI_GADGETMASK)
                    {                     
                        case IDBSAVEANDCLOSE:

                            GetAttr(STRINGA_TextVal, co[0], (ULONG*)&vLocation);
                            sprintf(location, "%s", vLocation);

                            GetAttr(CHOOSER_Selected, co[1], (ULONG*)&iUnit);
                            sprintf(unit, "%s", iUnit==0 ? "metric":"imperial");

                            GetAttr(CHOOSER_Selected, co[2], (ULONG*)&iLang);
                            sprintf(lang, "%s", clanguages[iLang]);
					        
                            GetAttr(STRINGA_TextVal, co[3], (ULONG*)&vDays);
                            sprintf(days, "%s", vDays);

                            GetAttr(STRINGA_TextVal, co[4], (ULONG*)&vMonths);
                            sprintf(months, "%s", vMonths);

                            GetAttr(STRINGA_TextVal, co[5], (ULONG*)&vApikey);
                            sprintf(apikey, "%s", vApikey);

                            GetAttr(GA_Selected, co[6], &dIcon);
                            sprintf(displayIcon, dIcon ? "1":"0");

                            GetAttr(GA_Selected, co[7], &dLocation);
                            sprintf(displayLocation, dLocation ? "1":"0");

                            GetAttr(GA_Selected, co[8], &dDescription);
                            sprintf(displayDescription, dDescription ? "1":"0");

                            GetAttr(GA_Selected, co[9], &dDatetime);
                            sprintf(displayDateTime, dDatetime ? "1":"0");

                            GetAttr(INTEGER_Number, co[10], &addX); 
                            GetAttr(INTEGER_Number, co[11], &addY); 
                            GetAttr(INTEGER_Number, co[12], &addH);

                            SavePrefs();
                            
                            strcpy(tm, months);
                            strcpy(td, days);

                            amonths= getArray(tm, ",", 12);
                            adays = getArray(td, ",", 7);

                            // get weather URL
                            strcpy(weatherURL, getURL(apikey, location, unit, lang));
                            update();

                            donecreatePreferences = TRUE;
                            
                        break;
						
						case IDBCANCEL:
                            donecreatePreferences = TRUE;
                        break;
						
                        default:
                        break;
                    }
                break;
				
				default:
                break;
			}   
		}   
	}

	RA_CloseWindow(createPreferencesWinObj);
	createPreferencesWin=NULL;
	if (createPreferencesWinObj)
    {   
		DisposeObject(createPreferencesWinObj);
        createPreferencesWinObj = NULL;
    }
}

BOOL IsAddedToUserStartup(void)
{
    char satir[PATH_MAX];
    BOOL added = FALSE;
    fp = Open(USERSTARTUP, MODE_OLDFILE);
	
    if (fp)
	{
        while (FGets(fp, satir, sizeof(satir))) 
        {
            if (strstr(satir, APPNAME) != NULL)
            {
                added = TRUE;
                break;
            }
        }       
        Close(fp);
    }
    return added;
}

void RemoveLineFromUserStartup(void)
{
    BPTR fpn;
    char cmd[PATH_MAX+32];
    char satir[PATH_MAX];

    // The user start-up file was backed up in this drawer with the named 'User-StartupBACKUP'. And then flushes User-Startup file.
    sprintf(cmd, "copy %s to %sBACKUP QUIET >NIL: && echo %s > %s", USERSTARTUP, USERSTARTUP, " ", USERSTARTUP);      
    Execute(cmd, NULL, NULL);

    fp = Open(USERSTARTUP"BACKUP", MODE_OLDFILE);
    fpn = Open(USERSTARTUP, MODE_OLDFILE);

    if (fp && fpn)
	{   
        while (FGets(fp, satir, sizeof(satir))) 
        {
            if (strstr(satir, APPNAME) == NULL)
            {
                FPuts(fpn, satir);
            }
        }
        
        Close(fp);
        Close(fpn);
    }
    
}