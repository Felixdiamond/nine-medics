#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <map>
#include <limits>
#include <sstream>

enum class Mode {
    ADD,
    EDIT
};

class MedicationReminder
{
private:
    struct Medication
    {
        int id;
        std::string name;
        std::string dosage;
        std::vector<std::pair<int, int>> scheduledTimes;
        int remainingDoses;
        int refillThreshold;
    };

    std::map<int, Medication> medications;
    int nextId;
    bool running;

    int generateUniqueId()
    {
        return nextId++;
    }

    void saveMedications()
    {
        std::ofstream file("medications.txt");
        if (!file)
        {
            std::cout << "Error: Unable to open file for saving.\n";
            return;
        }
        file << nextId << "\n";  // Save the next ID to use
        for (const auto& [id, med] : medications)
        {
            file << id << "," << med.name << "," << med.dosage << "," << med.remainingDoses << "," << med.refillThreshold;
            for (const auto& time : med.scheduledTimes)
            {
                file << "," << time.first << "," << time.second;
            }
            file << "\n";
        }
        file.close();
        std::cout << "Medications saved to file.\n";
    }

    void loadMedications()
    {
        std::ifstream file("medications.txt");
        if (!file)
        {
            std::cout << "No saved medications found.\n";
            nextId = 1;  // Start with ID 1 if no file exists
            return;
        }
        file >> nextId;  // Read the next ID to use
        file.ignore();  // Ignore the newline after nextId

        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string token;
            Medication med;

            std::getline(iss, token, ',');
            med.id = std::stoi(token);
            std::getline(iss, med.name, ',');
            std::getline(iss, med.dosage, ',');
            std::getline(iss, token, ',');
            med.remainingDoses = std::stoi(token);
            std::getline(iss, token, ',');
            med.refillThreshold = std::stoi(token);

            med.scheduledTimes.clear();
            while (std::getline(iss, token, ','))
            {
                int hour = std::stoi(token);
                std::getline(iss, token, ',');
                int minute = std::stoi(token);
                med.scheduledTimes.push_back({hour, minute});
            }

            medications[med.id] = med;
        }
        file.close();
        std::cout << "Medications loaded from file.\n";
    }

    void playSound() {
        system("(speaker-test -t sine -f 1000)& pid=$!; sleep 1.0s; kill -9 $pid");
    }

    void checkReminders()
    {
        while (running)
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            struct tm *parts = std::localtime(&now_c);

            for (auto& [id, med] : medications)
            {
                for (const auto& time : med.scheduledTimes)
                {
                    if (parts->tm_hour == time.first && parts->tm_min == time.second)
                    {
                        this->playSound();
                        std::cout << "\n\nREMINDER: Time to take " << med.name << " - " << med.dosage << "\n\n";
                        med.remainingDoses--;

                        if (med.remainingDoses <= med.refillThreshold)
                        {
                            std::cout << "WARNING: " << med.name << " is running low. Please refill soon.\n";
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::minutes(1));
        }
    }

    // Function to validate input for integers
    int ValidateInputHandler(const std::string& prompt, Mode currentMode, bool allowEmpty = false) {
        std::string input;
        while (true) {
            std::cout << prompt;
            std::getline(std::cin, input);
            
            if (input.empty() && allowEmpty && currentMode == Mode::EDIT) {
                return std::numeric_limits<int>::min();
            }
            
            try {
                return std::stoi(input);
            } catch (const std::exception&) {
                std::cout << "Invalid input. Please enter a number";
                if (allowEmpty && currentMode == Mode::EDIT) {
                    std::cout << " or press Enter to keep the existing value";
                }
                std::cout << ".\n";
            }
        }
    }

    // Function to validate number range
    int ValidateInputHandler(const std::string& prompt, Mode currentMode, int min, int max, bool allowEmpty = false) {
        while (true) {
            int input = ValidateInputHandler(prompt, currentMode, allowEmpty);
            if (input == std::numeric_limits<int>::min()) {
                return input; // Return sentinel value for empty input
            }
            if (input >= min && input <= max) {
                return input;
            } else {
                std::cout << "Invalid input. Please enter a number between " << min << " and " << max;
                if (allowEmpty && currentMode == Mode::EDIT) {
                    std::cout << " or press Enter to keep the existing value";
                }
                std::cout << ".\n";
            }
        }
    }

public:
    MedicationReminder() : running(true), nextId(1) {}

    void addMedication()
    {
        Medication med;
        med.id = generateUniqueId();
        std::cout << "\n\nEnter medication name: ";
        std::getline(std::cin >> std::ws, med.name);

        // input validation for dosage
        while (true)
        {
            std::cout << "Enter dosage: (i.e 1 pill, 2 tablets, etc. number only) ";
            std::getline(std::cin >> std::ws, med.dosage);
            if (med.dosage.find_first_not_of("0123456789") == std::string::npos)
            {
                break;
            }
            else
            {
                std::cout << "Invalid input. Please enter a number only.\n";
            }
        }

        int timesPerDay = ValidateInputHandler("Enter number of times per day: ", Mode::ADD);
        med.scheduledTimes.clear();
        for (int i = 0; i < timesPerDay; i++)
        {
            int hour = ValidateInputHandler("Enter hour to take (0-23) for time " + std::to_string(i + 1) + ": ", Mode::ADD, 0, 23);
            int minute = ValidateInputHandler("Enter minute to take (0-59) for time " + std::to_string(i + 1) + ": ", Mode::ADD, 0, 59);
            med.scheduledTimes.push_back({hour, minute});
        }
        med.remainingDoses = ValidateInputHandler("Enter remaining doses: ", Mode::ADD);
        med.refillThreshold = ValidateInputHandler("Enter refill threshold: ", Mode::ADD);

        medications[med.id] = med;
        saveMedications();
        std::cout << "Medication '" << med.name << "' (ID: " << med.id << ") has been added and saved.\n";
    }

    void updateMedication()
    {
        displayMedications();
        int id = ValidateInputHandler("Enter the ID of the medication to update: ", Mode::EDIT);
        
        auto it = medications.find(id);
        if (it == medications.end())
        {
            std::cout << "Medication with ID " << id << " not found.\n";
            return;
        }

        Medication& med = it->second;
        Mode currentMode = Mode::EDIT;

        std::cout << "Current dosage: " << med.dosage << "\n";
        std::string newDosage;
        while (true) {
            std::cout << "Enter new dosage (number only) or press Enter to keep current: ";
            std::getline(std::cin, newDosage);
            if (newDosage.empty()) {
                std::cout << "Keeping current dosage.\n";
                break;
            } else if (newDosage.find_first_not_of("0123456789") == std::string::npos) {
                med.dosage = newDosage;
                break;
            } else {
                std::cout << "Invalid input. Please enter a number only.\n";
            }
        }

        std::cout << "Current number of times per day: " << med.scheduledTimes.size() << "\n";
        int timesPerDay = ValidateInputHandler("Enter new number of times per day or press Enter to keep current: ", currentMode, true);
        if (timesPerDay != std::numeric_limits<int>::min()) {
            med.scheduledTimes.clear();
            for (int i = 0; i < timesPerDay; i++) {
                int hour = ValidateInputHandler("Enter hour to take (0-23) for time " + std::to_string(i + 1) + ": ", currentMode, 0, 23);
                int minute = ValidateInputHandler("Enter minute to take (0-59) for time " + std::to_string(i + 1) + ": ", currentMode, 0, 59);
                med.scheduledTimes.push_back({hour, minute});
            }
        }

        std::cout << "Current remaining doses: " << med.remainingDoses << "\n";
        int newRemainingDoses = ValidateInputHandler("Enter new remaining doses or press Enter to keep current: ", currentMode, true);
        if (newRemainingDoses != std::numeric_limits<int>::min()) {
            med.remainingDoses = newRemainingDoses;
        }

        std::cout << "Current refill threshold: " << med.refillThreshold << "\n";
        int newRefillThreshold = ValidateInputHandler("Enter new refill threshold or press Enter to keep current: ", currentMode, true);
        if (newRefillThreshold != std::numeric_limits<int>::min()) {
            med.refillThreshold = newRefillThreshold;
        }

        saveMedications();
        std::cout << "Medication '" << med.name << "' (ID: " << med.id << ") has been updated and saved.\n";
    }

    void deleteMedication()
    {
        displayMedications();
        int id = ValidateInputHandler("Enter the ID of the medication to delete: ", Mode::EDIT);
        
        auto it = medications.find(id);
        if (it == medications.end())
        {
            std::cout << "Medication with ID " << id << " not found.\n";
            return;
        }

        std::string name = it->second.name;
        medications.erase(it);
        saveMedications();
        std::cout << "Medication '" << name << "' (ID: " << id << ") has been deleted.\n";
    }

    void displayMedications()
    {
        if (medications.empty())
        {
            std::cout << "\nNo medications available.\n";
            return;
        }
        std::cout << "\n\nMedications:";
        for (const auto& [id, med] : medications)
        {
            std::cout << "\n\nID: " << id << "\n";
            std::cout << "Name: " << med.name << "\n";
            std::cout << "Dosage: " << med.dosage << "\n";
            std::cout << "Remaining Doses: " << med.remainingDoses << "\n";
            std::cout << "Refill Threshold: " << med.refillThreshold << "\n";
            std::cout << "Scheduled Times:";
            for (const auto& [hour, minute] : med.scheduledTimes)
            {
                std::cout << "  " << hour << ":" << std::setfill('0') << std::setw(2) << minute << "  ";
            }
            std::cout << "\n";
        }
    }

    void start()
    {
        loadMedications();
        std::thread reminderThread(&MedicationReminder::checkReminders, this);
        
        while (running)
        {
            std::cout << "\nMedication Reminder Menu:\n";
            std::cout << "1. Add Medication\n";
            std::cout << "2. Update Medication\n";
            std::cout << "3. Delete Medication\n";
            std::cout << "4. Display Medications\n";
            std::cout << "5. Exit\n";
            std::cout << "Enter your choice: ";

            int choice = ValidateInputHandler("", Mode::ADD);

            switch (choice)
            {
            case 1:
                addMedication();
                break;
            case 2:
                updateMedication();
                break;
            case 3:
                deleteMedication();
                break;
            case 4:
                displayMedications();
                break;
            case 5:
                running = false;
                break;
            default:
                std::cout << "Invalid choice. Please try again.\n";
            }
        }

        reminderThread.join();
    }
};

int main()
{
    MedicationReminder app;
    app.start();
    return 0;
}