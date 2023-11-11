//My first "serious" Qualcomm BREW program!
//Honestly? The whole programming model isn't too horrific.
//The whole "builtin_func(void *builtin_func_obj,...)" is REALLY stupid (I think this is
//some sort of object oriented artifact?) and the C compiler under VS 2005 is completely
//braindead (thats why this is a "cpp" file). But I did atleast enjoy programming half of this program!
//
//I'll get better with time. Practice makes perfect!

#include "AEEModGen.h"
#include "AEEAppGen.h"
#include "AEEShell.h"
#include "AEEHeap.h"
#include "Cgameoflife.h"

//MALLOCing large chunks of memory causes the SPH-M330 to shit its pants.
#define WIDTH 128
//#define HEIGHT 147
#define HEIGHT 128

typedef struct _gameoflife {
    AEEApplet  applet;
    IDisplay * piDisplay;
    IShell   * piShell;
    AEEDeviceInfo  deviceInfo;
} gameoflife;

//Some globals.
AEERect rect_tmp;
gameoflife *yes;
char *buffer_a;
char *buffer_b;

//Function declarations.
static  boolean gameoflife_HandleEvent(gameoflife* pMe,AEEEvent eCode,uint16 wParam, uint32 dwParam);
boolean gameoflife_InitAppData(gameoflife* pMe);
void    gameoflife_FreeAppData(gameoflife* pMe);
static void gameoflife_main(gameoflife * pMe);
static void loop(gameoflife * pMe);

int AEEClsCreateInstance(AEECLSID ClsId, IShell * piShell, IModule * piModule,void ** ppObj)
{
    *ppObj = NULL;
    if(AEECLSID_CGAMEOFLIFE==ClsId) {
	    if(TRUE==AEEApplet_New(sizeof(gameoflife),ClsId,piShell,piModule,(IApplet**)ppObj,(AEEHANDLER)gameoflife_HandleEvent,(PFNFREEAPPDATA)gameoflife_FreeAppData)){
		    if(TRUE == gameoflife_InitAppData((gameoflife*)*ppObj)) {
			    return AEE_SUCCESS;
			}else{
                IApplet_Release((IApplet*)*ppObj);
                return AEE_EFAILED;
            }
        }
    }
    return AEE_EFAILED;
}
boolean gameoflife_InitAppData(gameoflife * pMe)
{
    pMe->piDisplay = pMe->applet.m_pIDisplay;
    pMe->piShell   = pMe->applet.m_pIShell;
    pMe->deviceInfo.wStructSize = sizeof(pMe->deviceInfo);
    ISHELL_GetDeviceInfo(pMe->applet.m_pIShell,&pMe->deviceInfo);
	//malloc the buffers
	buffer_a=(char*)MALLOC(sizeof(char)*128*128);
	buffer_b=(char*)MALLOC(sizeof(char)*128*128);
	//My gut instinct is telling me that memory safety under Qualcomm BREW is piss poor,
	//so make sure MALLOC succeeded and didn't return null pointers!
	if(buffer_a==NULL||buffer_b==NULL){
		return FALSE;
	}
    return TRUE;
}
void gameoflife_FreeAppData(gameoflife * pMe)
{
	//While desktop operating systems are smart enough to clean up after a program after it
	//exits, we can't be sure the AMSS does the same.
	FREE(buffer_a);
	FREE(buffer_b);
}
static boolean gameoflife_HandleEvent(gameoflife* pMe,AEEEvent eCode, uint16 wParam, uint32 dwParam)
{  
    switch (eCode) {
        case EVT_APP_START:
			//"A journey of a 1000 miles begins with a single step. So hit the power!"
            gameoflife_main(pMe);
            return TRUE;
        case EVT_APP_STOP:
      	    return TRUE;
        case EVT_APP_SUSPEND:
			//AMSS has sent us a SIGSTOP, so clear the timer.
			ISHELL_CancelTimer(pMe->piShell,0,pMe);
      	    return TRUE;
        case EVT_APP_RESUME:
			//We do NOT want to restart the whole program when AMSS sends SIGCONT to us!
            ISHELL_SetTimer(pMe->piShell,10,(PFNNOTIFY)loop,pMe);
      	    return TRUE;
        case EVT_APP_MESSAGE:
      	    return TRUE;
        case EVT_KEY:
      	    return FALSE;
        case EVT_FLIP:
			//Keeping a CPU intensive process running while the phone is shut sounds like
			//an extremely inconsiderate thing so do. Especially when many of these phones
			//are ~15 years old and have batteries that are bloated/high hour/on their last
			//legs...
			ISHELL_CancelTimer(pMe->piShell,0,pMe);
            return TRUE;
        case EVT_KEYGUARD:
            return TRUE;
        default:
            break;
    }
    return FALSE; // Event wasn't handled.
}
static void clear_screen(){
	IDisplay_ClearScreen(yes->piDisplay);
}
static void update_screen(){
	IDisplay_Update(yes->piDisplay);
}
static void draw_pixel(uint16 x, uint16 y){
	rect_tmp.x=x;
	rect_tmp.y=y;
	IDisplay_DrawRect(yes->piDisplay,&rect_tmp,RGB_BLACK,RGB_BLACK,IDF_RECT_FILL);
}
static void draw_board(){
	clear_screen();
	for(uint16 x=0;x<WIDTH;x++){
		for(uint16 y=0;y<HEIGHT;y++){
			if(buffer_a[((x*HEIGHT)+y)]==1){
				draw_pixel(x,y);
			}
		}
	}
	update_screen();
}
static char get_pixel(int x, int y){
	if(x<0){
		x=WIDTH-1;
	}
	if(y<0){
		y=HEIGHT-1;
	}
	if(x>(WIDTH-1)){
		x=0;
	}
	if(y>(HEIGHT-1)){
		y=0;
	}
	return buffer_a[((x*HEIGHT)+y)];
}
static void step_conway(){
	char count;
	for(int x=0;x<WIDTH;x++){
		for(int y=0;y<HEIGHT;y++){
			count=0;
			count+=get_pixel(x+1,y+1);
			count+=get_pixel(x+1,y-1);
			count+=get_pixel(x-1,y+1);
			count+=get_pixel(x-1,y-1);
			count+=get_pixel(x+1,y);
			count+=get_pixel(x-1,y);
			count+=get_pixel(x,y+1);
			count+=get_pixel(x,y-1);
			if(count==3||(count==2&&get_pixel(x,y))){
				buffer_b[((x*HEIGHT)+y)]=1;
			}else{
				buffer_b[((x*HEIGHT)+y)]=0;
			}
		}
	}
}
char *tmp_ptr;
static void loop(gameoflife * pMe){
	//Appearantly doing step_conway(buffer_a,buffer_b) then step_conway(buffer_b,buffer_a) results in it stepping twice between screen draws...
	draw_board();
	step_conway();
	tmp_ptr=buffer_a;
	buffer_a=buffer_b;
	buffer_b=tmp_ptr;
	ISHELL_SetTimer(pMe->piShell,10,(PFNNOTIFY) loop,pMe);
}
static void gameoflife_main(gameoflife * pMe)
{
	yes=pMe;
	
	byte tmp[1];
	rect_tmp.dx=1;
	rect_tmp.dy=1;
	for(int i=0;i<WIDTH*HEIGHT;i++){
		GETRAND(tmp,1);
		if(tmp[0]%2==0){
			buffer_a[i]=1;
		}else{
			buffer_a[i]=0;
		}
	}
	loop(pMe);
}