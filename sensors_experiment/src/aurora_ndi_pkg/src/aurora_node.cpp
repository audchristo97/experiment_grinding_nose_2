#include <chrono>
#include <thread>
#include <functional>
#include <memory>
#include <string>

#ifdef _WIN32
#define ACCESS _access
#include <conio.h>   // for _kbhit()
#include <io.h>      // for _access_s()
#include <windows.h> // for Sleep(ms)
#else
#define ACCESS access
#include <unistd.h> // for POSIX sleep(sec), and access()
#include <sys/ioctl.h>
#endif

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "std_msgs/msg/string.hpp"

#include "CombinedApi.h"
#include "PortHandleInfo.h"
#include "ToolData.h"

static CombinedApi capi = CombinedApi();
static bool apiSupportsBX2 = false;
static bool apiSupportsStreaming = false;
// static bool tracking_mode = false; // Variable to monitor tracking state
static bool useEncryption = false;
static std::string cipher = "";
static bool useUDP = false;

std::string hostname = "/dev/ttyUSB0"; // Host name (IP address or PC USB port)
// Add your ROM files in the tools array
std::string tools[1] = {"/home/a-tamby/experiment_grinding_node/sensors_experiment/aurora_api/sroms/DDRO-1000-2367-01_GENERIC_5D.rom"};
ToolData toolData; // Object to store sensor data
float array_tool_data[7] = {};

// If you need more tools, add them into the tool array

using namespace std::chrono_literals;

/* This example creates a subclass of Node and uses std::bind() to register a
 * member function as a callback from the timer. */

class Aurora : public rclcpp::Node
{
public:
    Aurora()
        : Node("aurora_talker")
    {

        // Connect Aurora to ROS2 network
        aurora_connect();

        // Initialise system: clear all previously loaded tools, unsaved settings, etc.
        onErrorPrintDebugMessage("capi.initialize()", capi.initialize());

        // // Load Aurora Tool Definition Files and initialise the tools
        // loading_aurora_sroms();

        // Once loaded, initialize and enable the tools
        std::vector<ToolData> enabledTools = std::vector<ToolData>();
        initializeAndEnableTools(enabledTools);

        // Print an error if no tools were specified
        if (enabledTools.size() == 0)
        {
            std::cout << "No tools detected. To load passive tools, specify: --tools=[tool1.rom],[tool2.rom]" << std::endl;
        }

        // Start tracking mode
        std::cout << std::endl
                  << "Entering tracking mode..." << std::endl;
        onErrorPrintDebugMessage("capi.startTracking()", capi.startTracking());

        tracking_mode_ = true;

        aurora_thread_ = std::thread(&Aurora::tracking_transform, this);

        //publisher_ = this->create_publisher<std_msgs::msg::String>("aurora_transform", 10);
        aurora_publisher_ = this->create_publisher<std_msgs::msg::Float64MultiArray>("aurora_transform",10);
        timer_ = this->create_wall_timer(
            10ms, std::bind(&Aurora::timer_callback, this));
    }

    // static void cleanup()
    // {
    //     std::cout << std::endl
    //               << "Leaving tracking mode and returning to configuration mode..." << std::endl;
    //     onErrorPrintDebugMessage("capi.stopTracking()", capi.stopTracking());
    // }

    ~Aurora()
    {
        tracking_mode_ = false;
        // std::cout << std::endl
        //           << "Leaving tracking mode and returning to configuration mode..." << std::endl;
        // onErrorPrintDebugMessage("capi.stopTracking()", capi.stopTracking());
        if (aurora_thread_.joinable())
        {
            aurora_thread_.join();
        }
        std::cout << std::endl
                  << "Leaving tracking mode and returning to configuration mode..." << std::endl;
        onErrorPrintDebugMessage("capi.stopTracking()", capi.stopTracking());
    }

private:
    void tracking_transform()
    {
        while (tracking_mode_)
        {
            //std::lock_guard<std::mutex> lock(data_mutex_);
            //toolData_ = capi.getTrackingDataTX();
            toolData_ = capi.getTrackingDataBX()[0];
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    /*
     * @brief Function to connect the Aurora system to the ROS2 network
     */
    void aurora_connect(void)
    {
        // Start connection with the Aurora System
        if (capi.connect(hostname, useEncryption ? Protocol::SecureTCP : Protocol::TCP, cipher) != 0)
        {
            // Print the error and exit if we can't connect to a device
            std::cout << "Connection Failed!" << std::endl;
            std::cout << "Press Enter to continue...";
            std::cin.ignore();
        }
        std::cout << "Connected!" << std::endl;

        // Wait a second - needed to support connecting to LEMO vega
        sleepSeconds(1);

        // Print the firmware version for debugging purposes
        std::cout << capi.getUserParameter("Features.Firmware.Version") << std::endl;

        // Check if the device support BX2
        determineApiSupportForBX2();
    }

    /*
     * @brief Function to load the Aurora Tool Definition Files to the Aurora system
     */
    void loading_aurora_sroms(void)
    {
        // Size of the tools array based on the number of the rom files you included earlier
        int tools_size = sizeof(tools) / sizeof(tools[0]);

        if (tools_size > 0)
        {
            std::cout << "Loading tool definition (.rom files)..." << std::endl;
            for (int i = 0; i < tools_size; i++)
            {
                std::cout << "Loading: " << tools[i] << std::endl;
                loadTool(tools[i].c_str());
            }
        }
    }

    /*
     * @brief Equivalent of std::filesystem::exists() that works with -std=c++11
     */
    bool fileExists(const std::string &Filename)
    {
        return ACCESS(Filename.c_str(), 0) == 0;
    }

    /**
     * @brief There's no standard cross platform sleep() keystroke detection
     */
    int detectKeystroke()
    {
#ifdef _WIN32
        return _kbhit();
#else
        int bytesWaiting;
        ioctl(0, FIONREAD, &bytesWaiting); // stdin=0
        return bytesWaiting;
#endif
    }

    /**
     * @brief There's no standard cross platform sleep() method prior to C++11
     */
    void sleepSeconds(unsigned numSeconds)
    {
#ifdef _WIN32tool
        Sleep((DWORD)1000 * numSeconds); // Sleep(ms)
#else
        sleep(numSeconds); // sleep(sec)
#endif
    }

    /**
     * @brief Prints a debug message if a method call failed.
     * @details To use, pass the method name and the error code returned by the method.
     *          Eg: onErrorPrintDebugMessage("capi.initialize()", capi.initialize());
     *          If the call succeeds, this method does nothing.
     *          If the call fails, this method prints an error message to stdout.
     */
    void onErrorPrintDebugMessage(std::string methodName, int errorCode)
    {
        if (errorCode < 0)
        {
            std::cout << methodName << " failed: " << capi.errorToString(errorCode) << std::endl;
        }
    }

    /**
     * @brief Returns the string: "[tool.id] s/n:[tool.serialNumber]" used in CSV output
     */
    std::string getToolInfo(std::string toolHandle)
    {
        // Get the port handle info from PHINF toolData.transform.q0
        PortHandleInfo info = capi.portHandleInfo(toolHandle);

        // Return the ID and SerialNumber the desired string format
        std::string outputString = info.getToolId();
        outputString.append(" s/n:").append(info.getSerialNumber());
        return outputString;
    }

    /**
     * @brief Returns a string representation of the data in CSV format.
     * @details The CSV format is: "Frame#,ToolHandle,Face,TransformStatus,q0,qx,qy,qz,tx,ty,tz,error,#markers,[Marker1:status,x,y,z],[Marker2..."
     */
    std::string toolDataToCSV(const ToolData &toolData)
    {
        std::stringstream stream;
        stream << std::setprecision(toolData.PRECISION) << std::setfill('0');
        stream << "" << static_cast<unsigned>(toolData.frameNumber) << ","
               << "Port:" << static_cast<unsigned>(toolData.transform.toolHandle) << ",";
        stream << static_cast<unsigned>(toolData.transform.getFaceNumber()) << ",";

        if (toolData.transform.isMissing())
        {
            stream << "Missing,,,,,,,,";
        }
        else
        {
            stream << TransformStatus::toString(toolData.transform.getErrorCode()) << ","
                   << toolData.transform.q0 << "," << toolData.transform.qx << "," << toolData.transform.qy << "," << toolData.transform.qz << ","
                   << toolData.transform.tx << "," << toolData.transform.ty << "," << toolData.transform.tz << "," << toolData.transform.error;
        }

        // Each marker is printed as: status,tx,ty,tz
        stream << "," << toolData.markers.size();
        for (int i = 0; i < toolData.markers.size(); i++)
        {
            stream << "," << MarkerStatus::toString(toolData.markers[i].status);
            if (toolData.markers[i].status == MarkerStatus::Missing)
            {
                stream << ",,,";
            }
            else
            {
                stream << "," << toolData.markers[i].x << "," << toolData.markers[i].y << "," << toolData.markers[i].z;
            }
        }
        return stream.str();
    }

    /**
     * @brief Write tracking data to a CSV file in the format: "#Tools,ToolInfo,Frame#,[Tool1],Frame#,[Tool2]..."
     * @details It's worth noting that the number lines in the file does not necessarily match the number of frames collected.
     *          NDI measurement systems support different types of tools: passive, active, and active-wireless.
     *          Because different tool types are detected in different physical ways, each tool type has a separate frame for
     *          collecting data for all tools of that type. Each line of the file has the same number of tools, but each tool
     *          may have a different frame number that corresponds to its tool type.
     * @param fileName The file to write to.
     * @param numberOfLines The number of lines to write.
     * @param enabledTools ToolData storing the serial number for each enabled tool.
     */
    void writeCSV(std::string fileName, int numberOfLines, std::vector<ToolData> &enabledTools)
    {
        // Print header information to the first line of the output file
        std::cout << std::endl
                  << "Writing CSV file..." << std::endl;
        std::ofstream csvFile(fileName.c_str());
        csvFile << "#Tools";

        // Loop to gather tracking data and write to the file
        int linesWritten = 0;
        int previousFrameNumber = 0; // use this variable to avoid printing duplicate data with BX
        while (linesWritten < numberOfLines)
        {
            // Get new tool data using BX2
            std::vector<ToolData> newToolData = apiSupportsBX2 ? capi.getTrackingDataBX2("--6d=tools --3d=tools --sensor=none --1d=buttons") : capi.getTrackingDataBX(TrackingReplyOption::TransformData | TrackingReplyOption::AllTransforms);

            // Update enabledTools array with new data
            for (int t = 0; t < enabledTools.size(); t++)
            {
                for (int j = 0; j < newToolData.size(); j++)
                {
                    if (enabledTools[t].transform.toolHandle == newToolData[j].transform.toolHandle)
                    {
                        // Copy the new tool data
                        newToolData[j].toolInfo = enabledTools[t].toolInfo; // keep the serial number
                        enabledTools[t] = newToolData[j];                   // use the new data
                    }
                }
            }

            // If we're using BX2 there's extra work to do because BX2 and BX use opposite philosophies.
            // BX will always return data for all enabled tools, but some of the data may be old: #linesWritten == # BX commands
            // BX2 never returns old data, but cannot guarantee new data for all enabled tools with each call: #linesWritten <= # BX2 commands
            // We want a CSV file with data for all enabled tools in each line, but this requires multiple BX2 calls.
            if (apiSupportsBX2)
            {
                // Count the number of tools that have new data
                int newDataCount = 0;
                for (int t = 0; t < enabledTools.size(); t++)
                {
                    if (enabledTools[t].dataIsNew)
                    {
                        newDataCount++;
                    }
                }

                // Send another BX2 if some tools still have old data
                if (newDataCount < enabledTools.size())
                {
                    continue;
                }
            }
            else
            {
                if (previousFrameNumber == enabledTools[0].frameNumber)
                {
                    // If the frame number didn't change, don't print duplicate data to the CSV, send another BX
                    continue;
                }
                else
                {
                    // This frame number is different, so we'll print a line to the CSV, but remember it for next time
                    previousFrameNumber = enabledTools[0].frameNumber;
                }
            }

            // If this is the first line of the CSV, print labels for tool and marker data
            if (linesWritten == 0)
            {
                for (int t = 0; t < enabledTools.size(); t++)
                {
                    csvFile << ",ToolInfo,Frame#,PortHandle,Face#,TransformStatus,Q0,Qx,Qy,Qz,Tx,Ty,Tz,Error,#Markers";
                    for (int m = 0; m < enabledTools[t].markers.size(); m++)
                    {
                        csvFile << ",Marker" << m << ".Status,Tx,Ty,Tz";
                    }
                }
                csvFile << std::endl;
            }

            // Print a line of the CSV file if all enabled tools have new data
            csvFile << std::dec << enabledTools.size();
            for (int t = 0; t < enabledTools.size(); t++)
            {
                csvFile << "," << enabledTools[t].toolInfo << "," << toolDataToCSV(enabledTools[t]);
                enabledTools[t].dataIsNew = false; // once printed, the data becomes "old"
            }
            csvFile << std::endl;
            linesWritten++;
        }
    }

    /**
     * @brief Prints a ToolData object to stdout
     * @param toolData The data to print
     */
    void printToolData(const ToolData &toolData)
    {
        if (toolData.systemAlerts.size() > 0)
        {
            std::cout << "[" << toolData.systemAlerts.size() << " alerts] ";
            for (int a = 0; a < toolData.systemAlerts.size(); a++)
            {
                std::cout << toolData.systemAlerts[a].toString() << std::endl;
            }
        }

        if (toolData.buttons.size() > 0)
        {
            std::cout << "[buttons: ";
            for (int b = 0; b < toolData.buttons.size(); b++)
            {
                std::cout << ButtonState::toString(toolData.buttons[b]) << " ";
            }
            std::cout << "] ";
        }
        std::cout << toolDataToCSV(toolData) << std::endl;
    }

    /**
     * @brief Put the system into tracking mode, and poll a few frames of data.
     */
    void printTrackingData()
    {
        // Start tracking, output a few frames of data, and stop tracking

        for (int i = 0; i < 10; i++)
        {
            // Demonstrate TX command: ASCII command sent, ASCII reply received~Aurora()
            // {   tracking_mode_ = false;
            //     std::cout << std::endl
            //               << "Leaving tracking mode and returning to configuration mode..." << std::endl;
            //     capi.stopTracking();
            // }
            std::cout << capi.getTrackingDataTX() << std::endl;

            // Demonstrate BX or BX2 command
            std::vector<ToolData> toolData = apiSupportsBX2 ? capi.getTrackingDataBX2() : capi.getTrackingDataBX();

            // Print to stdout in similar format to CSV
            std::cout << "[alerts] [buttons] Frame#,ToolHandle,Face#,TransformStatus,Q0,Qx,Qy,Qz,Tx,Ty,Tz,Error,#Markers,State,Tx,Ty,Tz" << std::endl;
            for (int i = 0; i < toolData.size(); i++)
            {
                printToolData(toolData[i]);
            }
        }
    }

    /**
     * @brief Initialize and enable loaded tools. This is the same regardless of tool type.
     */
    void initializeAndEnableTools(std::vector<ToolData> &enabledTools)
    {
        std::cout << std::endl
                  << "Initializing and enabling tools..." << std::endl;

        // Initialize and enable tools
        std::vector<PortHandleInfo> portHandles = capi.portHandleSearchRequest(PortHandleSearchRequestOption::NotInit);
        for (int i = 0; i < portHandles.size(); i++)
        {
            onErrorPrintDebugMessage("capi.portHandleInitialize()", capi.portHandleInitialize(portHandles[i].getPortHandle()));
            onErrorPrintDebugMessage("capi.portHandleEnable()", capi.portHandleEnable(portHandles[i].getPortHandle()));
        }

        // Print all enabled tools
        portHandles = capi.portHandleSearchRequest(PortHandleSearchRequestOption::Enabled);
        for (int i = 0; i < portHandles.size(); i++)
        {
            std::cout << portHandles[i].toString() << std::endl;
        }

        // Lookup and store the serial number for each enabled tool
        for (int i = 0; i < portHandles.size(); i++)
        {
            enabledTools.push_back(ToolData());
            enabledTools.back().transform.toolHandle = (uint16_t)capi.stringToInt(portHandles[i].getPortHandle());
            enabledTools.back().toolInfo = getToolInfo(portHandles[i].getPortHandle());
        }
    }

    /**
     * @brief Loads a tool from a tool definition file (.rom)
     */
    void loadTool(const char *toolDefinitionFilePath)
    {
        // Request a port handle to load a passive tool into
        int portHandle = capi.portHandleRequest();
        onErrorPrintDebugMessage("capi.portHandleRequest()", portHandle);

        // Load the .rom file using the previously obtained port handle
        capi.loadSromToPort(toolDefinitionFilePath, portHandle);
    }

    /**
     * @brief Demonstrate detecting active tools.
     * @details Active tools are connected through a System Control Unit (SCU) with physical wires.
     */
    void configureActiveTools(std::string scuHostname)
    {
        // Setup the SCU connection for demonstrating active tools
        std::cout << std::endl
                  << "Configuring Active Tools - Setup SCU Connection" << std::endl;
        onErrorPrintDebugMessage("capi.setUserParameter()", capi.setUserParameter("Param.Connect.SCU Hostname", scuHostname));
        std::cout << capi.getUserParameter("Param.Connect.SCU Hostname") << std::endl;

        // Wait a few seconds for the SCU to detect any wired tools plugged in
        std::cout << std::endl
                  << "Demo Active Tools - Detecting Tools..." << std::endl;
        sleepSeconds(2);

        // Print all port handles
        std::vector<PortHandleInfo> portHandles = capi.portHandleSearchRequest(PortHandleSearchRequestOption::NotInit);
        for (int i = 0; i < portHandles.size(); i++)
        {
            std::cout << portHandles[i].toString() << std::endl;
        }
    }

    /**
     * @brief Demonstrate loading an active wireless tool.
     * @details Active wireless tools are battery powered and emit IR in response to a chirp from the illuminators.
     */
    void configureActiveWirelessTools()
    {
        // Load an active wireless tool definitions from a .rom files
        std::cout << std::endl
                  << "Configuring an Active Wireless Tool - Loading .rom File..." << std::endl;
        loadTool("sroms/active-wireless.rom");
    }

    /**
     * @brief Demonstrate loading dummy tools of each tool type.
     * @details Dummy tools are used to report 3Ds in the absence of real tools.
     *          Dummy tools should not be loaded with regular tools of the same type.
     *          TSTART will fail if real and dummy tools are enabled simultaneously.
     */
    void configureDummyTools()
    {
        std::cout << std::endl
                  << "Loading passive, active-wireless, and active dummy tools..." << std::endl;
        onErrorPrintDebugMessage("capi.loadPassiveDummyTool()", capi.loadPassiveDummyTool());
        onErrorPrintDebugMessage("capi.loadActiveWirelessDummyTool()", capi.loadActiveWirelessDummyTool());
        onErrorPrintDebugMessage("capi.loadActiveDummyTool()", capi.loadActiveDummyTool());
    }

    /**
     * @brief Demonstrate getting/setting user parameters.== 1
     */
    void configureUserParameters()
    {
        std::cout << capi.getUserParameter("Param.User.String0") << std::endl;
        onErrorPrintDebugMessage("capi.setUserParameter(Param.User.String0, customString)", capi.setUserParameter("Param.User.String0", "customString"));
        std::cout << capi.getUserParameter("Param.User.String0") << std::endl;
        onErrorPrintDebugMessage("capi.setUserParameter(Param.User.String0, emptyString)", capi.setUserParameter("Param.User.String0", ""));
    }

    /**
     * @brief Sets the user parameter "Param.Simulated Alerts" to test communication of system alerts.
     * @details This method does nothing if simulatedAlerts is set to 0x00000000.
     */
    void simulateAlerts(uint32_t simulatedAlerts = 0x00000000)
    {
        // Simulate alerts if any were requested
        if (simulatedAlerts > 0x0000)
        {
            std::cout << std::endl
                      << "Simulating system alerts..." << std::endl;
            std::stringstream stream;
            stream << simulatedAlerts;
            onErrorPrintDebugMessage("capi.setUserParameter(Param.Simulated Alerts, alerts)", capi.setUserParameter("Param.Simulated Alerts", stream.str()));
            std::cout << capi.getUserParameter("Param.Simulated Alerts") << std::endl;
        }
    }

    /**
     * @brief Determines whether an NDI device supports the BX2 command by looking at the API revision
     */
    void determineApiSupportForBX2()
    {
        // Lookup the API revision
        std::string response = capi.getApiRevision();

        // Refer to the API guide for how to interpret the APIREV response
        char deviceFamily = response[0];
        int majorVersion = capi.stringToInt(response.substr(2, 3));

        // As of early 2017, the only NDI device supporting BX2 is the Vega
        // Vega is a Polaris device with API major version 003
        if (deviceFamily == 'G' && majorVersion >= 3)
        {
            apiSupportsBX2 = true;
            apiSupportsStreaming = true;
        }
    }

    void timer_callback()
    {

        // message.data = std::vector<double>(array_tool_data, array_tool_data + 6);
        // RCLCPP_INFO(this->get_logger(), "Publishing: '%d'", message.data);
        // publisher_->publish(message);

        // DO NOTHING
        //auto message = std_msgs::msg::String();
        auto message = std_msgs::msg::Float64MultiArray();
        //{
            //std::lock_guard<std::mutex> lock(data_mutex_);
        std::vector<double> data_aurora = {toolData_.transform.q0, toolData_.transform.qx, toolData_.transform.qy, toolData_.transform.qz, toolData_.transform.tx, toolData_.transform.ty, toolData_.transform.tz};
        message.data = data_aurora;
        //}
        //RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", message.data.c_str());
        //publisher_->publish(message);
        aurora_publisher_->publish(message);
    }

    rclcpp::TimerBase::SharedPtr timer_;
    //rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr aurora_publisher_;
    std::thread aurora_thread_;
    std::mutex data_mutex_;
    std::atomic<bool> tracking_mode_ = false;
    // toolData_;
    ToolData toolData_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    // rclcpp::on_shutdown(Aurora::cleanup);
    rclcpp::spin(std::make_shared<Aurora>());
    rclcpp::shutdown();
    return 0;
}