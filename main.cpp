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
#include <nlohmann/json.hpp>

enum class Mode
{
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

        nlohmann::json toJSON() const
        {
            nlohmann::json j;
            j["id"] = id;
            j["name"] = name;
            j["dosage"] = dosage;
            j["remainingDoses"] = remainingDoses;
            j["refillThreshold"] = refillThreshold;

            for (const auto &time : scheduledTimes)
            {
                j["scheduledTimes"].push_back({time.first, time.second});
            }

            return j;
        }

        static Medication fromJSON(const nlohmann::json &j)
        {
            Medication med;
            med.id = j.at("id").get<int>();
            med.name = j.at("name").get<std::string>();
            med.dosage = j.at("dosage").get<std::string>();
            med.remainingDoses = j.at("remainingDoses").get<int>();
            med.refillThreshold = j.at("refillThreshold").get<int>();

            for (const auto &time : j.at("scheduledTimes"))
            {
                med.scheduledTimes.push_back({time[0], time[1]});
            }

            return med;
        }
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
        nlohmann::json j;
        j["nextId"] = nextId;

        for (const auto &[id, med] : medications)
        {
            j["medications"].push_back(med.toJSON());
        }

        std::ofstream file("medications.json");
        if (!file)
        {
            std::cout << "Error: Unable to open file for saving.\n";
            return;
        }
        file << j.dump(4);
        file.close();
        std::cout << "Medications saved to medications.json.\n";
    }

    void loadMedications()
    {
        std::ifstream file("medications.json");
        if (!file)
        {
            std::cout << "No saved medications found.\n";
            nextId = 1;
            return;
        }

        nlohmann::json j;
        file >> j;
        file.close();

        nextId = j.at("nextId").get<int>();
        for (const auto &medJson : j.at("medications"))
        {
            Medication med = Medication::fromJSON(medJson);
            medications[med.id] = med;
        }

        std::cout << "Medications loaded from medications.json.\n";
    }

    void playSound()
    {
        system("(speaker-test -t sine -f 1000)& pid=$!; sleep 1.0s; kill -9 $pid");
    }

    void checkReminders()
    {
        while (running)
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            struct tm *parts = std::localtime(&now_c);

            for (auto &[id, med] : medications)
            {
                for (const auto &time : med.scheduledTimes)
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
    int ValidateInputHandler(const std::string &prompt, Mode currentMode, bool allowEmpty = false)
    {
        std::string input;
        while (true)
        {
            std::cout << prompt;
            std::getline(std::cin, input);

            if (input.empty() && allowEmpty && currentMode == Mode::EDIT)
            {
                return std::numeric_limits<int>::min();
            }

            try
            {
                return std::stoi(input);
            }
            catch (const std::exception &)
            {
                std::cout << "Invalid input. Please enter a number";
                if (allowEmpty && currentMode == Mode::EDIT)
                {
                    std::cout << " or press Enter to keep the existing value";
                }
                std::cout << ".\n";
            }
        }
    }

    // Function to validate number range
    int ValidateInputHandler(const std::string &prompt, Mode currentMode, int min, int max, bool allowEmpty = false)
    {
        while (true)
        {
            int input = ValidateInputHandler(prompt, currentMode, allowEmpty);
            if (input == std::numeric_limits<int>::min())
            {
                return input; // Return sentinel value for empty input
            }
            if (input >= min && input <= max)
            {
                return input;
            }
            else
            {
                std::cout << "Invalid input. Please enter a number between " << min << " and " << max;
                if (allowEmpty && currentMode == Mode::EDIT)
                {
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

        Medication &med = it->second;
        Mode currentMode = Mode::EDIT;

        std::cout << "Current dosage: " << med.dosage << "\n";
        std::string newDosage;
        while (true)
        {
            std::cout << "Enter new dosage (number only) or press Enter to keep current: ";
            std::getline(std::cin, newDosage);
            if (newDosage.empty())
            {
                std::cout << "Keeping current dosage.\n";
                break;
            }
            else if (newDosage.find_first_not_of("0123456789") == std::string::npos)
            {
                med.dosage = newDosage;
                break;
            }
            else
            {
                std::cout << "Invalid input. Please enter a number only.\n";
            }
        }

        std::cout << "Current number of times per day: " << med.scheduledTimes.size() << "\n";
        int timesPerDay = ValidateInputHandler("Enter new number of times per day or press Enter to keep current: ", currentMode, true);
        if (timesPerDay != std::numeric_limits<int>::min())
        {
            med.scheduledTimes.clear();
            for (int i = 0; i < timesPerDay; i++)
            {
                int hour = ValidateInputHandler("Enter hour to take (0-23) for time " + std::to_string(i + 1) + ": ", currentMode, 0, 23);
                int minute = ValidateInputHandler("Enter minute to take (0-59) for time " + std::to_string(i + 1) + ": ", currentMode, 0, 59);
                med.scheduledTimes.push_back({hour, minute});
            }
        }

        std::cout << "Current remaining doses: " << med.remainingDoses << "\n";
        int newRemainingDoses = ValidateInputHandler("Enter new remaining doses or press Enter to keep current: ", currentMode, true);
        if (newRemainingDoses != std::numeric_limits<int>::min())
        {
            med.remainingDoses = newRemainingDoses;
        }

        std::cout << "Current refill threshold: " << med.refillThreshold << "\n";
        int newRefillThreshold = ValidateInputHandler("Enter new refill threshold or press Enter to keep current: ", currentMode, true);
        if (newRefillThreshold != std::numeric_limits<int>::min())
        {
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

        // Header
        std::cout << "\n"
                  << std::string(60, '=') << "\n";
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "Name"
                  << std::setw(10) << "Dosage"
                  << std::setw(10) << "Remaining"
                  << std::setw(10) << "Refill"
                  << "Scheduled Times"
                  << "\n";
        std::cout << std::string(60, '=') << "\n";

        // Medications data
        for (const auto &[id, med] : medications)
        {
            std::cout << std::left << std::setw(5) << id
                      << std::setw(20) << med.name
                      << std::setw(10) << med.dosage
                      << std::setw(10) << med.remainingDoses
                      << std::setw(10) << med.refillThreshold;

            // Display scheduled times
            for (const auto &[hour, minute] : med.scheduledTimes)
            {
                std::cout << std::setfill('0') << std::setw(2) << hour << ":"
                          << std::setw(2) << minute << "  ";
            }

            std::cout << "\n";
        }

        std::cout << std::string(60, '=') << "\n";
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