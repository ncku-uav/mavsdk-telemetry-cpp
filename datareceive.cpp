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

static void get_position(SOCKET sclient,std::shared_ptr<System> system,sockaddr_in sin,int len);
static void get_eulerangle(SOCKET sclient,std::shared_ptr<System> system,sockaddr_in sin,int len);

    // static void get_position(std::shared_ptr<System> system);
    // static void get_eulerangle(std::shared_ptr<System> system);




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

    // while (!telemetry.health_all_ok()) {
    //     std::cout << "Waiting for system to be ready\n";
    //     sleep_for(seconds(1));
    // }
    // std::cout << "System is ready\n";

    const Telemetry::Result set_rate_result = telemetry.set_rate_position(50);

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
    sin.sin_port = htons(50015);
    /*sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");*/
    inet_pton(AF_INET, "127.0.0.1", (void*)&sin.sin_addr.S_un.S_addr);

    int len = sizeof(sin);    
    //socket setup

    std::vector<std::thread> threads;

    // std::thread position (&get_position,std::ref(system));
    // threads.push_back(std::move(position));
    // std::thread eulerangle (&get_eulerangle,std::ref(system));
    // threads.push_back(std::move(eulerangle));

    std::thread position (&get_position,sclient,std::ref(system),sin,len);
    // threads.push_back(std::move(position));
    std::thread eulerangle (&get_eulerangle,sclient,std::ref(system),sin,len);
    // threads.push_back(std::move(eulerangle));



    // auto position = std::async (std::launch::async,get_position,sclient,telemetry,sin,len);
 
    // get_position(sclient,std::ref(system),sin,len);
    // get_eulerangle(sclient,std::ref(system),sin,len);
    // for (auto& position : threads) {
        position.join();
    // }
    // for (auto& eulerangle : threads) {
        eulerangle.join();
    // }


    return 0;       

}

void get_position(SOCKET sclient,std::shared_ptr<System> system,sockaddr_in sin,int len) {
    auto telemetry = Telemetry{system};
    time_t timer;
    std::string data,latval_data,lngval_data;
    std::string timeStamp;
    data = "latVal,0.3," + timeStamp;
    const char* sendData = "rollVal,10,";
    const char* data_send;
    std::cout<<"in position function";
    while(1){
        telemetry.subscribe_position([&sclient,&sin,&len,&data,&latval_data,&timeStamp,&lngval_data,&data_send,&timer](Telemetry::Position position) {
                timer = time(NULL);
                timeStamp = std::to_string(timer);
                latval_data = std::to_string(position.latitude_deg);
                lngval_data = std::to_string(position.longitude_deg);
                //data gain

                data = "latVal," + latval_data + ',' + timeStamp;
                data_send = data.c_str();
                sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
                //latitude data send

                data = "lngVal," + lngval_data + ',' + timeStamp;
                data_send = data.c_str();
                sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
                //longitude data send


                std::cout << "Vehicle is at: " << position.latitude_deg << ", " << position.longitude_deg
                        << " degrees\n";

        });   
    } 
    

}

void get_eulerangle(SOCKET sclient,std::shared_ptr<System> system,sockaddr_in sin,int len) {
    auto telemetry = Telemetry{system};   
    time_t timer;
    std::string data,rollval_data,pitchval_data,yawval_data;
    std::string timeStamp;
    // data = "latVal,0.3," + timeStamp;
    //const char* sendData = "rollVal,10,";
    const char* data_send;    
    std::cout<<"in euler function";

    while(1) {
        telemetry.subscribe_attitude_euler([&sclient,&sin,&len,&data,&rollval_data,&timeStamp,&pitchval_data,&yawval_data,&data_send,&timer](Telemetry::EulerAngle euler_angle) {
            timer = time(NULL);
            timeStamp = std::to_string(timer);  
            rollval_data = std::to_string(euler_angle.roll_deg);
            pitchval_data = std::to_string(euler_angle.pitch_deg);
            yawval_data = std::to_string(euler_angle.yaw_deg);
            //data gain

            data = "rollVal," + rollval_data + ',' + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //roll data send

            std::cout<<"roll data is: " << data;            

            data = "pitchVal," + pitchval_data + ',' + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //pitch data send

            std::cout<<"pitch data is: " << data;            

            data = "yawVal," + yawval_data + ',' + timeStamp;
            data_send = data.c_str();
            sendto(sclient, data_send, strlen(data_send), 0, (sockaddr*)&sin, len);
            //yaw data send

            // std::cout << "Angle is: " << euler_angle.roll_deg << ", " << euler_angle.pitch_deg << ", " << euler_angle.yaw_deg
            //         << " degrees\n";
            std::cout<<"yaw data is: " << data;

        });
    }


}


