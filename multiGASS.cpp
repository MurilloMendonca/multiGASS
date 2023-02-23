#include "newGASS.hpp"
#include <vector>

using namespace std;
namespace multiGASS{
    float runWithThreads(int num_threads, std::vector<site>& templates, std::vector<Repositorio>& repositorios){
        std::vector<std::thread> threads;
        auto start = std::chrono::high_resolution_clock::now();
        std::mutex mtx;
        int i=0;
        for(int i=0;i<num_threads;i++){
            threads.push_back(std::thread([](std::vector<site>& templates, std::vector<Repositorio>& repositorios, std::mutex& mtx, int& i){
                while(repositorios.size()>i){
                    mtx.lock();
                    if(repositorios.size()==i){
                        mtx.unlock();
                        return;
                    }
                    Repositorio repositorio = repositorios[i++];
                    mtx.unlock();
                    GASS::run(std::ref(templates), std::ref(repositorio));
                }
            },std::ref(templates),std::ref(repositorios), std::ref(mtx), std::ref(i)));
        }
        for(auto& thread : threads)
            thread.join();
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        return elapsed.count();
    }

    void setup(string setupFilePath, string templatesFilePath,
             string substitutionsFilePath, string runListFileName,
             string templateFolderPath, vector<site>& templates, vector<Repositorio>& repositorios){
                GASS::readConfigFile(setupFilePath);
                GASS::readSetupTemplateFile(templatesFilePath, templates);
                for(site& s : templates)
                    GASS::readTemplateFile(templateFolderPath+s.pdbId, s);
                GASS::readSubstitutuionMatrix(substitutionsFilePath, templates);

                vector<string> proteinNames;
                GASS::readRunFile(runListFileName, proteinNames);
                repositorios.resize(proteinNames.size());
                for(int i=0;i<proteinNames.size();i++)
                    repositorios[i].readRepository("../cache/" + proteinNames[i] + "/targ_.dat");

    }
    void benchmark(std::vector<int> num_threads, std::string templatesFolderName, std::string runListFileName, std::string outputFileName){
        std::vector<site> templates;
        std::vector<Repositorio> repositorios;
        multiGASS::setup(templatesFolderName+"GA_Conf.txt", templatesFolderName+"Templates.txt", 
                        templatesFolderName+"SubstitutionMatrix.txt",runListFileName,
                        templatesFolderName,templates, repositorios);
        std::fstream file;
        file.open(outputFileName, std::ios::out);
        for(int i=num_threads.size()-1;i>=0;i--){
            std::cout<<"Running with "<<num_threads[i]<<" threads"<<std::endl;
            float elapsed = runWithThreads(num_threads[i], templates, repositorios);
            std::cout<<"Elapsed time: "<<elapsed<<std::endl;
            file<<num_threads[i]<<","<<elapsed<<std::endl;
        }
        file.close();
    }
}


int main(){
    std::vector<int> num_threads = {5};
    std::string templatesFolderName = "../templates/Templates_Fe/";
    std::string runListFileName = "benchmark100.txt";
    multiGASS::benchmark(num_threads, templatesFolderName, runListFileName, "benchmark100Geral.csv");
}