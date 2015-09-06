#include "portaudio.hpp"
#include <iostream>
#include <string>

int main()
{
    std::string input;

    Pa_Initialize();
    while(std::getline(std::cin, input)){
        int count = Pa_GetDeviceCount();
        std::cout << count << std::endl;
        for(int i = 0;i < count;i++){
            auto info = Pa_GetDeviceInfo(i);
            std::cout << info->name << std::endl;
        }    
        if(input == "quit") break;
    }
    Pa_Terminate();

    while(std::getline(std::cin, input)){
        Pa_Initialize();
        int count = Pa_GetDeviceCount();
        std::cout << count << std::endl;
        for(int i = 0;i < count;i++){
            auto info = Pa_GetDeviceInfo(i);
            std::cout << info->name << std::endl;
        }    
        Pa_Terminate();
    }

}
