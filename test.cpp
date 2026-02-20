#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <oci.h>

// Standard Curl callback to capture web data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to extract slugs from the raw "Master List" JSON
// Note: For full accuracy, a JSON library is best, but we'll use string parsing 
// to keep your setup simple without adding more libraries today.
std::vector<std::string> extractSlugs(const std::string& rawJson) {
    std::vector<std::string> slugs;
    // The problems endpoint uses "titleSlug":"
    std::string key = "\"titleSlug\":\"";
    size_t pos = rawJson.find(key);
    
    while (pos != std::string::npos) {
        size_t start = pos + key.length();
        size_t end = rawJson.find("\"", start);
        if (end != std::string::npos) {
            std::string slug = rawJson.substr(start, end - start);
            slugs.push_back(slug);
        }
        pos = rawJson.find(key, end);
    }

    if (slugs.empty()) {
        std::cout << "!! Debug: Still no slugs. Total JSON chars: " << rawJson.length() << std::endl;
        if(rawJson.length() > 200) std::cout << "Snippet: " << rawJson.substr(0, 200) << std::endl;
    }
    std::cout << "Server Response: " << rawJson << std::endl;
    
    return slugs;
}

int main() {
    std::cout << "--- AlgoForge: FULL AUTOMATION MODE ---" << std::endl;

    // 1. FETCH THE MASTER LIST FIRST
    std::string masterListJson;
    CURL* curl = curl_easy_init();
    if(curl) {
        std::cout << "[Step 1] Fetching master list of all problems..." << std::endl;
        // Change Step 1 URL to this:
curl_easy_setopt(curl, CURLOPT_URL, "https://alfa-leetcode-api.onrender.com/problems?limit=100&skip=200");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &masterListJson);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    std::vector<std::string> allSlugs = extractSlugs(masterListJson);
    int total = allSlugs.size();
    std::cout << "[Step 1] SUCCESS: Found " << total << " problems to download." << std::endl;

    // 2. ORACLE INITIALIZATION
    OCIEnv *envhp; OCIError *errhp; OCISvcCtx *svchp;
    OCIEnvCreate(&envhp, OCI_DEFAULT, NULL, NULL, NULL, NULL, 0, NULL);
    OCIHandleAlloc(envhp, (void **)&errhp, OCI_HTYPE_ERROR, 0, NULL);
    
    std::cout << "[Step 2] Connecting to Oracle..." << std::endl;
    sword status = OCILogon(envhp, errhp, &svchp, (unsigned char*)"PRAKRITI", 8, (unsigned char*)"Prakriti076", 11, (unsigned char*)"localhost:1521/XEPDB1", 21);
    
    if (status != OCI_SUCCESS) {
        std::cout << "Database connection failed!" << std::endl;
        return 1;
    }

    // 3. RANDOM GENERATOR FOR JITTER
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> delayDist(5, 10); 

    // 4. THE AUTOMATED LOOP
    for (int i = 0; i < total; ++i) {
        std::string currentSlug = allSlugs[i];
        int questionNum = i + 1;
        int remaining = total - questionNum;

        // FETCH INDIVIDUAL PROBLEM DATA
        std::string problemData;
        curl = curl_easy_init();
        if(curl) {
            std::string url = "https://alfa-leetcode-api.onrender.com/select?titleSlug=" + currentSlug;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &problemData);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }

        // STORE IN DATABASE
        OCIStmt *stmthp;
        std::string sql = "INSERT INTO LEETCODE_PROBLEMS (TITLE, SLUG, RAW_JSON) VALUES (:t, :s, :j)";
        OCIHandleAlloc(envhp, (void **)&stmthp, OCI_HTYPE_STMT, 0, NULL);
        OCIStmtPrepare(stmthp, errhp, (unsigned char*)sql.c_str(), sql.length(), OCI_NTV_SYNTAX, OCI_DEFAULT);
        
        OCIBind *b1=NULL, *b2=NULL, *b3=NULL;
        OCIBindByName(stmthp, &b1, errhp, (unsigned char*)":t", -1, (void*)currentSlug.c_str(), currentSlug.length()+1, SQLT_STR, 0,0,0,0,0, OCI_DEFAULT);
        OCIBindByName(stmthp, &b2, errhp, (unsigned char*)":s", -1, (void*)currentSlug.c_str(), currentSlug.length()+1, SQLT_STR, 0,0,0,0,0, OCI_DEFAULT);
        OCIBindByName(stmthp, &b3, errhp, (unsigned char*)":j", -1, (void*)problemData.c_str(), problemData.length()+1, SQLT_STR, 0,0,0,0,0, OCI_DEFAULT);

        OCIStmtExecute(svchp, stmthp, errhp, 1, 0, NULL, NULL, OCI_COMMIT_ON_SUCCESS);

        // PROGRESS FEEDBACK
        std::cout << "\n>> Question number (" << questionNum << ") [" << currentSlug << "] stored successfully." << std::endl;
        std::cout << ">> (" << remaining << ") questions remain." << std::endl;

        // DYNAMIC COOLDOWN
        if (remaining > 0) {
            int waitTime = delayDist(gen);
            std::cout << "Going for question number (" << (questionNum + 1) << ") in: " << std::flush;
            for (int c = waitTime; c > 0; --c) {
                std::cout << c << "... " << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "RUN!" << std::endl;
        }
    }

    return 0;
}