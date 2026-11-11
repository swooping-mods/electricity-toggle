/* M06: rendering-only power-link toggle, executed on existing Unity callbacks. */
#define MOONLAB_LATE_UPDATE_ROLE 1
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/input.h"
/* Win64's floating-point object marker; this freestanding DLL uses SSE directly. */
int _fltused=0;
static PTR log_file,clear_method,typing_method,lines_field;
static PTR component_object,object_active,list_count,list_item;
static PTR world_field,restoring_method,blocked_method,focused_method,first_person_method;
static int hidden,ticks,last_sample=-1,failed,main_thread;
static int world_ready,ready_frames,last_world_status=-1;
static int test_tools,was_catalog_down,was_budget_down,was_turn_down,was_pitch_down;
static PTR money_convert,set_test_funds,test_shop_class,test_shop_open;
static PTR test_look,component_transform,transform_forward,transform_up;
static PTR builder_class,get_component,catalog_begin,catalog_set_open,catalog_is_open;
static PTR production_class,production_links,power_world,power_owner,position_method;
static PTR begin_method,end_method,link_method,world_links,anchor_method,building_method,holding_method;
static int nearby_links,last_overlay_sample=-1;
static unsigned char (*class_value_type)(PTR);
static PTR (*class_type)(PTR);
static PTR (*type_object)(PTR);
static short (*key_state)(int);
static PTR (*foreground)(void);
static DWORD (*window_pid)(PTR,DWORD *);
static BOOL (*window_title)(PTR,const char *);
static int text_equal(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void record(const char *event,int total,int active){
    char b[512],*p=b;p=cat(p,"{\"event\":\"");p=cat(p,event);p=cat(p,"\",\"mod\":\"electricity-toggle\",\"version\":\"0.2.0\",\"hidden\":");
    p=cat(p,hidden?"true":"false");p=cat(p,",\"ticks\":");p=num(p,(U64)ticks);p=cat(p,",\"thread\":");p=num(p,GetCurrentThreadId());
    p=cat(p,",\"total\":");p=num(p,(U64)total);p=cat(p,",\"active\":");p=num(p,(U64)active);p=cat(p,"}\n");
    DWORD written=0;WriteFile(log_file,b,(DWORD)(p-b),&written,0);FlushFileBuffers(log_file);
}
static void sample(PTR self,const char *event){
    int ok=1;PTR list=0;field_value(self,lines_field,&list);if(!list)return;
    /* Resolve generic List<T> methods through the actual object's class. */
    PTR klass=object_class(list);
    list_count=get_method(klass,"get_Count",0);list_item=get_method(klass,"get_Item",1);
    int total=integer(list_count,list,&ok),active=0;
    if(!ok||total<0||total>2048){failed=1;record("sample-failed",0,0);return;}
    for(int i=0;i<total;i++){
        void *args[1]={&i};PTR renderer=call(list_item,list,args,&ok);
        if(!renderer)continue;
        PTR obj=call(component_object,renderer,0,&ok);
        if(obj&&boolean(object_active,obj,&ok))active++;
    }
    if(!ok){failed=1;record("sample-failed",0,0);return;}
    record(event,total,active);
}
static PTR component(PTR walker,PTR klass,int *ok){
    PTR type=type_object(class_type(klass));void *args[1]={type};
    return type?call(get_component,walker,args,ok):0;
}
typedef struct {float x,y,z;} Vector3;
static float distance_squared(Vector3 a,Vector3 b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
static void walking_overlay(PTR walker){
    int ok=1;PTR world=0;field_value(walker,world_field,&world);
    if(!world||boolean(restoring_method,world,&ok)||boolean(blocked_method,walker,&ok)
       ||!boolean(first_person_method,walker,&ok)||!ok)return;
    PTR interaction=component(walker,production_class,&ok),links=0;
    if(interaction)field_value(interaction,production_links,&links);
    if(!ok||!links)return;
    if(hidden){call(clear_method,links,0,&ok);nearby_links=0;}
    else if(boolean(building_method,walker,&ok))return; /* Keep vanilla placement previews/rings. */
    else{
        PTR owner=0,link_world=0;field_value(links,power_owner,&owner);field_value(links,power_world,&link_world);
        if(!owner||link_world!=world)return;
        PTR boxed_position=call(position_method,owner,0,&ok),collection=call(world_links,world,0,&ok);
        if(!ok||!boxed_position||!collection)return;
        Vector3 position=*(Vector3 *)unbox(boxed_position);
        /* Use the collection's non-generic enumerator: no guessed List/HashSet layout. */
        PTR enumerator=call(get_method(object_class(collection),"System.Collections.IEnumerable.GetEnumerator",0),collection,0,&ok);
        if(!ok||!enumerator){failed=1;record("enumerator-failed",0,0);return;}
        PTR klass=object_class(enumerator),self=class_value_type(klass)?unbox(enumerator):enumerator;
        PTR move=get_method(klass,"MoveNext",0),current=get_method(klass,"get_Current",0),dispose=get_method(klass,"Dispose",0);
        call(begin_method,links,0,&ok);int scanned=0;nearby_links=0;
        while(ok&&boolean(move,self,&ok)){
            if(++scanned>2048){ok=0;break;}
            PTR pair=call(current,self,0,&ok);
            if(!pair){ok=0;break;}
            PTR pair_class=object_class(pair),first=get_field(pair_class,"Item1"),second=get_field(pair_class,"Item2");
            if(!first||!second){ok=0;break;}
            int a=0,b=0;field_value(pair,first,&a);field_value(pair,second,&b);
            Vector3 pa={0},pb={0};void *aa[2]={&a,&pa},*ab[2]={&b,&pb};
            PTR va=call(anchor_method,world,aa,&ok),vb=call(anchor_method,world,ab,&ok);
            if(!ok||!va||!vb){ok=0;break;}
            if(!*(unsigned char *)unbox(va)||!*(unsigned char *)unbox(vb))continue;
            if(distance_squared(pa,position)>4900.0f&&distance_squared(pb,position)>4900.0f)continue;
            float alpha=1.0f;void *args[3]={&a,&b,&alpha};call(link_method,links,args,&ok);nearby_links++;
        }
        if(dispose)call(dispose,self,0,&ok);
        call(end_method,links,0,&ok);
    }
    if(!ok){failed=1;hidden=0;record("overlay-failed",0,0);return;}
    if(ticks-last_overlay_sample>=30){
        PTR builder=component(walker,builder_class,&ok);
        int holding=builder?boolean(holding_method,builder,&ok):0;
        if(!ok){failed=1;record("tool-state-failed",0,0);return;}
        last_overlay_sample=ticks;sample(links,holding?"tool-sample":"walking-sample");
    }
}
static void open_catalog(PTR walker){
    int ok=1;PTR type=type_object(class_type(builder_class));void *args[1]={type};
    PTR builder=type?call(get_component,walker,args,&ok):0;
    if(!builder||!ok){record("catalog-open-failed",0,0);return;}
    /* Vanilla SetOpen requires the build-tool placement session first. */
    call(catalog_begin,builder,0,&ok);
    unsigned char yes=1;args[0]=&yes;call(catalog_set_open,builder,args,&ok);
    int opened=boolean(catalog_is_open,builder,&ok);
    record(!ok?"catalog-open-failed":opened?"catalog-opened":"catalog-warming",0,0);
}
static void test_budget(PTR walker){
    int ok=1;PTR world=0;field_value(walker,world_field,&world);int amount=5000;void *args[1]={&amount};
    PTR money=call(money_convert,0,args,&ok);
    if(!world||!money||!ok){record("test-budget-failed",0,0);return;}
    args[0]=unbox(money);call(set_test_funds,world,args,&ok);
    record(ok?"test-budget":"test-budget-failed",ok?amount:0,0);
    PTR shop=component(walker,test_shop_class,&ok);
    if(shop&&ok){unsigned char yes=1;args[0]=&yes;call(test_shop_open,shop,args,&ok);record(ok?"test-shop-opened":"test-shop-failed",0,0);}
    else record("test-shop-unavailable",0,0);
}
static void test_camera(PTR walker,int turn){
    int ok=1;PTR transform=call(component_transform,walker,0,&ok);
    PTR f=call(transform_forward,transform,0,&ok),u=call(transform_up,transform,0,&ok);
    if(!ok||!f||!u){record("test-camera-failed",0,0);return;}
    Vector3 forward=*(Vector3 *)unbox(f),up=*(Vector3 *)unbox(u),direction;
    if(turn)direction=(Vector3){up.y*forward.z-up.z*forward.y,up.z*forward.x-up.x*forward.z,up.x*forward.y-up.y*forward.x};
    else direction=(Vector3){forward.x-up.x*.6f,forward.y-up.y*.6f,forward.z-up.z*.6f};
    /* The game's TestLook takes a world-space Vector3 direction, not mouse pixels. */
    void *args[1]={&direction};call(test_look,walker,args,&ok);record(ok?"test-camera":"test-camera-failed",turn,0);
}
static int before(int id,Context *c){
    if(id==4){if(!failed&&world_ready&&(DWORD)main_thread==GetCurrentThreadId())walking_overlay((PTR)c->rcx);return 0;}
    if(id==0){
        if(!main_thread){main_thread=(int)GetCurrentThreadId();record("main-thread",0,0);}
        if((DWORD)main_thread!=GetCurrentThreadId()){failed=1;return 0;}
        ticks++;PTR window=foreground();DWORD pid=0;if(window)window_pid(window,&pid);
        if(!world_ready){
            PTR world=0;field_value((PTR)c->rcx,world_field,&world);
            if(world){
                int ok=1;
                int status=boolean(restoring_method,world,&ok)
                    |(boolean(blocked_method,(PTR)c->rcx,&ok)<<1)
                    |((!boolean(focused_method,0,&ok))<<2)
                    |((!boolean(first_person_method,(PTR)c->rcx,&ok))<<3)
                    |(boolean(typing_method,0,&ok)<<4);
                if(!ok)status|=32;
                if(status!=last_world_status){last_world_status=status;record("world-status",status,0);}
                if(!status&&ticks>120)ready_frames++;else ready_frames=0;
                if(ready_frames>=120){world_ready=1;record("world-ready",0,0);}
            }
        }
        int input_ok=1;int typing=boolean(typing_method,0,&input_ok);
        MoonInputState toggle=moon_input_action("electricity-toggle.toggle","F8",!failed&&pid==GetCurrentProcessId()&&input_ok&&!typing);
        int catalog_down=(key_state(0x76)&0x8000)!=0;
        int budget_down=(key_state(0x75)&0x8000)!=0;
        if(test_tools&&world_ready&&pid==GetCurrentProcessId()&&budget_down&&!was_budget_down)test_budget((PTR)c->rcx);
        was_budget_down=budget_down;
        int turn_down=(key_state(0x78)&0x8000)!=0,pitch_down=(key_state(0x79)&0x8000)!=0;
        if(test_tools&&world_ready&&pid==GetCurrentProcessId()){
            int ok=1;
            if(!boolean(blocked_method,(PTR)c->rcx,&ok)&&ok){
                if(turn_down&&!was_turn_down)test_camera((PTR)c->rcx,1);
                if(pitch_down&&!was_pitch_down)test_camera((PTR)c->rcx,0);
            }
        }
        was_turn_down=turn_down;was_pitch_down=pitch_down;
        if(test_tools&&world_ready&&pid==GetCurrentProcessId()&&catalog_down&&!was_catalog_down)open_catalog((PTR)c->rcx);
        was_catalog_down=catalog_down;
        if(toggle.pressed){hidden=!hidden;record("toggle",0,0);}
        if(pid==GetCurrentProcessId())window_title(window,failed?"Moon Sweepers | Electricity Toggle ERROR":hidden?"Moon Sweepers | Electricity Toggle 0.2.0 | F8: lines OFF":nearby_links?"Moon Sweepers | Electricity Toggle 0.2.0 | F8: lines ON":"Moon Sweepers | Electricity Toggle 0.2.0 | F8: lines ON | no connections nearby");
        return 0;
    }
    if(failed)return 0;
    if(hidden){
        int ok=1;call(clear_method,(PTR)c->rcx,0,&ok);
        if(!ok){failed=1;hidden=0;record("clear-failed",0,0);return 0;}
    }
    if(ticks-last_sample>=60){last_sample=ticks;sample((PTR)c->rcx,"sample");}
    return hidden&&!failed;
}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    char path[1000];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",path,900);if(!n||n>=900)return 0;
    cat(path+n,"\\electricity-toggle.jsonl");log_file=CreateFileA(path,0x40000000,1,0,1,0x80,0);
    if(log_file==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)){record("bind-failed",0,0);return 0;}
    PTR user=LoadLibraryA("user32.dll");if(!user)return 0;
    key_state=(short(*)(int))GetProcAddress(user,"GetAsyncKeyState");foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");
    window_pid=(DWORD(*)(PTR,DWORD *))GetProcAddress(user,"GetWindowThreadProcessId");window_title=(BOOL(*)(PTR,const char *))GetProcAddress(user,"SetWindowTextA");
    if(!key_state||!foreground||!window_pid||!window_title)return 0;
    PTR power=find_class("MoonCleaners","MoonPowerLinks");if(!power)return 0;
    clear_method=get_method(power,"Clear",0);lines_field=get_field(power,"lines");
    typing_method=method("MoonCleaners","MoonInput","get_Typing",0);
    world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");
    restoring_method=method("MoonCleaners","MoonWorld","get_IsRestoringRendering",0);
    blocked_method=method("MoonCleaners","MoonWalker","get_UIBlocked",0);
    focused_method=method("UnityEngine","Application","get_isFocused",0);
    first_person_method=method("MoonCleaners","MoonWalker","get_IsFirstPerson",0);
    component_object=method("UnityEngine","Component","get_gameObject",0);object_active=method("UnityEngine","GameObject","get_activeSelf",0);
    if(!clear_method||!lines_field||!typing_method||!component_object||!object_active||!world_field||!restoring_method||!blocked_method||!focused_method||!first_person_method)return 0;
    production_class=find_class("MoonCleaners","MoonProductionInteraction");
    production_links=get_field(production_class,"links");power_world=get_field(power,"world");power_owner=get_field(power,"owner");
    position_method=method("UnityEngine","Transform","get_position",0);
    begin_method=get_method(power,"Begin",0);end_method=get_method(power,"End",0);link_method=get_method(power,"Link",3);
    world_links=method("MoonCleaners","MoonWorld","get_PowerLinks",0);anchor_method=method("MoonCleaners","MoonWorld","PowerAnchor",2);
    building_method=method("MoonCleaners","MoonWalker","get_Building",0);
    class_value_type=(unsigned char(*)(PTR))GetProcAddress(game,"il2cpp_class_is_valuetype");
    if(!production_class||!production_links||!power_world||!power_owner||!position_method||!begin_method||!end_method
       ||!link_method||!world_links||!anchor_method||!building_method||!class_value_type)return 0;
    char mode[8];test_tools=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",mode,8)==1&&mode[0]=='1';
    {
        builder_class=find_class("MoonCleaners","MoonDepotBuilder");
        if(!builder_class)return 0;
        holding_method=get_method(builder_class,"get_HoldingBuildTool",0);
        if(!holding_method||method_token(holding_method)!=100663892)return 0;
        catalog_begin=get_method(builder_class,"BeginPlacement",0);
        catalog_set_open=get_method(builder_class,"SetOpen",1);catalog_is_open=get_method(builder_class,"get_IsOpen",0);
        class_type=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_class_get_type");
        type_object=(PTR(*)(PTR))GetProcAddress(game,"il2cpp_type_get_object");
        PTR (*methods)(PTR,PTR *)=(PTR(*)(PTR,PTR *))GetProcAddress(game,"il2cpp_class_get_methods");
        const char *(*name)(PTR)=(const char *(*)(PTR))GetProcAddress(game,"il2cpp_method_get_name");
        DWORD (*arity)(PTR)=(DWORD(*)(PTR))GetProcAddress(game,"il2cpp_method_get_param_count");
        PTR (*param)(PTR,DWORD)=(PTR(*)(PTR,DWORD))GetProcAddress(game,"il2cpp_method_get_param");
        char *(*type_name)(PTR)=(char *(*)(PTR))GetProcAddress(game,"il2cpp_type_get_name");
        void (*release)(PTR)=(void(*)(PTR))GetProcAddress(game,"il2cpp_free");
        if(!catalog_begin||!catalog_set_open||!catalog_is_open||!class_type||!type_object||!methods||!name||!arity||!param||!type_name||!release)return 0;
        PTR iter=0,m;while((m=methods(find_class("UnityEngine","Component"),&iter))){
            if(!text_equal(name(m),"GetComponent")||arity(m)!=1)continue;
            char *t=type_name(param(m,0));int match=t&&text_equal(t,"System.Type");if(t)release(t);
            if(match){get_component=m;break;}
        }
        if(!get_component)return 0;
        if(test_tools){
            test_look=method("MoonCleaners","MoonWalker","TestLook",1);
            component_transform=method("UnityEngine","Component","get_transform",0);
            transform_forward=method("UnityEngine","Transform","get_forward",0);transform_up=method("UnityEngine","Transform","get_up",0);
            set_test_funds=method("MoonCleaners","MoonWorld","SetTeamDollars",1);
            test_shop_class=find_class("MoonCleaners","MoonUpgradeShop");test_shop_open=get_method(test_shop_class,"SetOpen",1);
            PTR decimal=find_class("System","Decimal");iter=0;
            while(decimal&&(m=methods(decimal,&iter))){
                if(!text_equal(name(m),"op_Implicit")||arity(m)!=1)continue;
                char *t=type_name(param(m,0));int match=t&&text_equal(t,"System.Int32");if(t)release(t);
                if(match){money_convert=m;break;}
            }
            if(!money_convert||!set_test_funds||!test_shop_class||!test_shop_open||!test_look
               ||method_token(test_look)!=100671745||!component_transform||!transform_forward||!transform_up)return 0;
        }
    }
    if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game)
       ||!prepare_hook(get_method(power,"ShowAmbient",0),100668936,1,game)
       ||!prepare_hook(get_method(power,"ShowGrid",1),100668935,2,game)
       ||!prepare_hook(get_method(power,"ShowPreview",4),100668938,3,game)
       ||!prepare_hook(method("MoonCleaners","MoonWalker","LateUpdate",0),100671741,4,game)){
        record("hook-shape-failed",hook_count,0);return 0;
    }
    for(int i=0;i<hook_count;i++){
        char b[512],*p=b;p=cat(p,"{\"event\":\"binding\",\"id\":");p=num(p,(U64)hooks[i].id);p=cat(p,",\"rva\":");p=num(p,(U64)hooks[i].address-(U64)game);
        p=cat(p,",\"instruction_bytes\":[");for(int j=0;j<hooks[i].length;j++){if(j)p=cat(p,",");p=num(p,hooks[i].original[j]);}p=cat(p,"]}\n");DWORD written=0;WriteFile(log_file,b,(DWORD)(p-b),&written,0);
    }
    before_hook=before;if(!install_hooks()){record("hook-install-failed",0,0);return 0;}
    record("ready",hook_count,0);return 1;
}
