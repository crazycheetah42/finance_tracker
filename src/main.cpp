// Import modules
#include "imgui.h" // imgui for gui
#include "imgui_impl_glfw.h" // needed for initializing imgui
#include "imgui_impl_opengl3.h" // needed for initializing imgui
#include <GLFW/glfw3.h> // glfw tells os to create a window
#include <iostream> // Standard Library for Input/Output
#include <fstream>
#include <string> // Needed to handle dynamic button text strings
#define STB_IMAGE_IMPLEMENTATION // Tells the header to create the actual loading logic
#include "stb_image.h"           // Allows us to read PNG/JPG files into raw pixels


// A helper function that encrypts or decrypts a string using a secret key
std::string encryptDecrypt(std::string data) {
    char key = 'K'; // This is your secret key character! You can change this to any letter/symbol.
    std::string output = data;
    
    // Loop through every single character in the sentence
    for (size_t i = 0; i < data.size(); i++) {
        // The ^ symbol is the bitwise XOR operator in C++
        output[i] = data[i] ^ key;
    }
    
    return output;
}

int main() {
    if (!glfwInit()) return -1; // This initializes the library. If it fails (returns false / 0), we stop the program immediately by returning -1.

    GLFWwindow* window = glfwCreateWindow(800, 600, "Finance Tracker", nullptr, nullptr); // 1280x720 window with title 'Finance Tracker'
    if (window == nullptr) return -1;
    
    // 🖼️ LOADING AND SETTING THE WINDOW ICON
    GLFWimage images[1]; // Create an array to hold our icon data structure
    
    int channels; // Will store whether the image has transparency (RGBA) or not
    // stbi_load reads our PNG file and hands back the raw color pixel data
    images[0].pixels = stbi_load("icon.png", &images[0].width, &images[0].height, &channels, 4); 

    // Check if the image successfully loaded from your folder
    if (images[0].pixels != nullptr) {
        // Apply the loaded pixel data directly to our GLFW window
        glfwSetWindowIcon(window, 1, images);
        
        // Always free the raw pixel memory from RAM once GLFW has successfully copied it
        stbi_image_free(images[0].pixels);
    } else {
        std::cout << "Warning: Could not find or load icon.png!" << std::endl;
    }

    glfwMakeContextCurrent(window); // This makes sure that any 'drawing' commands that we execute will happen in the window we made.
    glfwSwapInterval(1); // Enables vsync (we don't need 1000 fps for a text based program)

    // The following code initializes ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // Dark theme setup

    // Tie ImGui to our GLFW window and OpenGL graphics
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Static variables defined BEFORE the loop so they don't reset 60 times a second
    static float amount = 0.0f; // Creates a floating point (decimal) variable. The 'static' part tells C++ to remember the value of the variable when the loop repeats
    static char description[128] = ""; // Creates a variable for adding a description to expenses (Limited to 127 characters since imgui is based on C)
    static bool isIncome = false; // This bool variable contains whether the input is income or not: false means Expense, true means Income

    while (!glfwWindowShouldClose(window)) { // This is important so that the window actually stays open and doesn't close immediately when you launch it
        glfwPollEvents(); // Handle OS events first
        
        // CRITICAL: Start the ImGui frame BEFORE calling ImGui::Begin()
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get the current window size dynamically in case the user resizes it
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Forces the ImGui panel to match the full window size and lock onto the top left corner (0,0)
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));

        // Start our master window panel with flags that turn off resizing/moving/titles so it fills the screen perfectly
        ImGui::Begin("Expense Tracker Dashboard", nullptr, 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        // Splits the screen into 2 vertical sections: Left column is inputs, Right column is dashboard/history
        ImGui::Columns(2, "MainLayout", true);
        
        // Setup a default starting width for our left-hand sidebar control panel
        static bool set_column_width = true;
        if (set_column_width) {
            ImGui::SetColumnWidth(0, 400.0f); // Make left column 350 pixels wide
            set_column_width = false;
        }
        ImGui::Text("Add New Transaction"); // Header Label
        ImGui::Separator(); // Adds a nice horizontal dividing line below header

        // A button that toggles our isIncome variable back and forth when clicked
        if (ImGui::Button(isIncome ? "Switch to Expense" : "Switch to Income")) {
            isIncome = !isIncome; // Flips the switch (true becomes false, false becomes true)
        }

        ImGui::Spacing(); // Adds some blank structural spacing

        ImGui::SetNextItemWidth(250.0f); // Shrinks the width of the text/number boxes so they look tidy in our sidebar
        ImGui::InputFloat("Transaction Amount", &amount); // the "&amount" passes the memory location of our variable to ImGui. This line makes an input.
        
        ImGui::SetNextItemWidth(250.0f);
        ImGui::InputText("Description", description, IM_ARRAYSIZE(description)); // New: Text input box for description

        ImGui::Spacing();

        // Dynamically changes the text on our action button depending on if the user selected income or expense
        std::string buttonText = isIncome ? "Add Income" : "Add Expense";
        if (ImGui::Button(buttonText.c_str())) { 
            std::ofstream outputFile("expenses.txt", std::ios::app);
            if (outputFile.is_open()) {
                float finalAmount = isIncome ? amount : -amount;

                // 1. Create a normal string line just like before
                std::string plainTextLine = std::string(description) + "," + std::to_string(finalAmount);

                // 2. Scramble the line using our encryption function
                std::string encryptedLine = encryptDecrypt(plainTextLine);

                // 3. Write the gibberish line into your text file
                outputFile << encryptedLine << "\n";
                outputFile.close(); 
                
                std::cout << "Successfully encrypted and saved!" << std::endl;

                description[0] = '\0'; 
                amount = 0.0f; 
            }
        }

        // Right side column (balance and history)
        ImGui::NextColumn(); // Tells ImGui to stop drawing on the left side and jump over to the right side column

        ImGui::Text("Financial Dashboard Overview"); // Dashboard Header Label
        ImGui::Separator();
        
        float totalBalance = 0.0f;

        std::ifstream inputFile("expenses.txt"); // Opening the file for reading
        std::string line;

        // FIRST PASS: Let's calculate the total balance
        if (inputFile.is_open()) {
            while (std::getline(inputFile, line)) {
                
                // 🔐 DECRYPT HERE FIRST: Turn gibberish back into readable text
                line = encryptDecrypt(line);

                size_t commaPos = line.find(',');
                if (commaPos != std::string::npos) {
                    std::string amountStr = line.substr(commaPos + 1);
                    totalBalance += std::stof(amountStr);
                }
            }
            inputFile.clear();
            inputFile.seekg(0);
        }
        ImGui::Text("Total Balance: $%.2f", totalBalance); // %.2f formatting ensures it always prints exactly 2 decimal places like currency
        
        ImGui::Spacing();
        ImGui::Text("Recent Transactions"); // History Header Label
        ImGui::Separator();
        
        if (ImGui::BeginChild("ScrollingHistoryRegion", ImVec2(0, 0), true)) {
            if (inputFile.is_open()) {
                while (std::getline(inputFile, line)) {
                    
                    // 🔐 DECRYPT HERE SECOND: Turn gibberish back into readable text
                    line = encryptDecrypt(line);

                    size_t commaPos = line.find(',');
                    if (commaPos != std::string::npos) {
                        std::string descStr = line.substr(0, commaPos);
                        std::string amountStr = line.substr(commaPos + 1);
                        float val = std::stof(amountStr);

                        if (val < 0.0f) {
                            ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s: -$%.2f", descStr.c_str(), -val);
                        } else {
                            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "%s: +$%.2f", descStr.c_str(), val);
                        }
                    }
                }
                inputFile.close();
            }
            ImGui::EndChild();
        }

        ImGui::Columns(1); // Safely resets the columns back to 1 before wrapping up the window code
        ImGui::End(); // ImGui::End() tells the engine we are finished adding components to "Expense Tracker Dashboard". Every Begin() needs an End()!
        
        ImGui::Render(); // Renders everything
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h); // Find out how big the window is
        glViewport(0, 0, display_w, display_h); // Set the drawing area to match the window
        
        glClearColor(0.3f, 0.3f, 0.32f, 1.0f); // Set a background color (slate gray)
        glClear(GL_COLOR_BUFFER_BIT); // 5. Clear the old frame out of memory
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Send the UI shapes to the graphics card

        glfwSwapBuffers(window); // 'double buffering' which draws the next frame in the background so that screen flickering doesn't occur
    }
    glfwDestroyWindow(window); // Destroys window when you close it and frees up RAM and GPU memory
    glfwTerminate();
    return 0;
}