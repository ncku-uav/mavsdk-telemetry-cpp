#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <stdio.h>   
#include <winsock2.h>   
#include <WS2tcpip.h>
#include <time.h>
#include <string>

#pragma comment(lib,"ws2_32.lib")    



using namespace mavsdk;
using std::this_thread::sleep_for;
using std::chrono::seconds;

void usage(const std::string& bin_name)
{
    std::cerr << "Usage : " << bin_name << " <connection_url>\n"
              << "Connection URL format should be :\n"
              << " For TCP : tcp://[server_host][:server_port]\n"
              << " For UDP : udp://[bind_host][:bind_port]\n"
              << " For Serial : serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}

std::shared_ptr<System> get_system(Mavsdk& mavsdk)
{
    std::cout << "Waiting to discover system...\n";
    auto prom = std::promise<std::shared_ptr<System>>{};
    auto fut = prom.get_future();

    // We wait for new systems to be discovered, once we find one that has an
    // autopilot, we decide to use it.
    mavsdk.subscribe_on_new_system([&mavsdk, &prom]() {
        auto system = mavsdk.systems().back();

        if (system->has_autopilot()) {
            std::cout << "Discovered autopilot\n";

            // Unsubscribe again as we only want to find one system.
            mavsdk.subscribe_on_new_system(nullptr);
            prom.set_value(system);
        }
    });

    // We usually receive heartbeats at 1Hz, therefore we should find a
    // system after around 3 seconds max, surely.
    // if (fut.wait_for(seconds(3)) == std::future_status::timeout) {
    //     std::cerr << "No autopilot found.\n";
    //     return {};
    // }

    // Get discovered system now.
    return fut.get();
}


int get_position(SOCKET sclient,Telemetry telemetry,sockaddr_in sin,int len) {
        time_t timer;
        std::string data,latval_data,lngval_data;
        std::string timeStamp;

        const char* data_send;

    while(1) {
        timer = time(NULL);
        timeStamp = std::to_string(timer);
        telemetry.subscribe_position([](Telemetry::Position position) {
            latval_data = to_string(position.latitude_deg);
            lngval_data = to_string(position.longitude_deg);
            //data gain

            data = "latVal," + latval_data + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //latitude data send

            data = "lngVal," + lngval_data + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //longitude data send



        });    
    }
    return 1;
}

int get_eulerangle(SOCKET sclient,Telemetry telemetry,sockaddr_in sin,int len) {
    time_t timer;
    std::string data,rollval_data,pitchval_data,yawval_data;
    std::string timeStamp;
    // data = "latVal,0.3," + timeStamp;
    //const char* sendData = "rollVal,10,";
    const char* data_send;    

    while(1) {
        timer = time(NULL);
        timeStamp = std::to_string(timer);        
        telemetry.subscribe_attitude_euler([](Telemetry::EulerAngle euler_angle) {

            rollval_data = to_string(euler_angle.roll_deg);
            pitchval_data = to_string(euler_angle.pitch_deg);
            yawval_data = to_string(euler_angle.yaw_deg);
            //data gain

            data = "rollVal," + rollval_data + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //roll data send

            std::cout<<"roll data is: " << data;            

            data = "pitchVal," + pitchval_data + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //pitch data send

            std::cout<<"pitch data is: " << data;            

            data = "yawVal," + yawval_data + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //yaw data send

            // std::cout << "Angle is: " << euler_angle.roll_deg << ", " << euler_angle.pitch_deg << ", " << euler_angle.yaw_deg
            //         << " degrees\n";
            std::cout<<"yaw data is: " << data;

            });
    }
    return 1;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    Mavsdk mavsdk;
    ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    auto system = get_system(mavsdk);
    if (!system) {
        return 1;
    }

    auto telemetry = Telemetry{system};



    const Telemetry::Result set_rate_result = telemetry.set_rate_position(50);
    // const Telemetry::Result set_rate_result = telemetry.set_rate_euler();
    if (set_rate_result != Telemetry::Result::Success) {
        std::cerr << "Setting rate failed:" << set_rate_result << '\n';
        return 1;
    }
    
    

    WORD socketVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(socketVersion, &wsaData) != 0)
    {
        return 0;
    }
    SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(50016);
    /*sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");*/
    inet_pton(AF_INET, "127.0.0.1", (void*)&sin.sin_addr.S_un.S_addr);

    int len = sizeof(sin);    
    //socket setup


    std::future<int> position = std::async (get_position,sclient,telemetry,sin,len);
    std::future<int> eulerangle = std::async (get_eulerangle,sclient,telemetry,sin,len);
        


    return 0;       

}