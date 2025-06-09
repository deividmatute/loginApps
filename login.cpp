#include <iostream>
#include <string>
#include <fstream>      // Para operaciones de archivo (std::ifstream, std::ofstream)
#include <vector>       // Para std::vector
#include <algorithm>    // Para std::transform (para convertir a minúsculas)
#include <cctype>       // Para ::tolower
#include <limits>       // Para std::numeric_limits
#include <cstdlib>      // Para std::system
#include <cstdio>       // Para _popen, _pclose (Windows specific for system command output)
#include <memory>       // Para std::unique_ptr
#include <stdexcept>    // Para std::runtime_error

// --- Configuración de Archivos y URLs ---
const std::string LOCAL_WORD_FILE = "local_secret_word.txt";
const std::string CLOUD_URL_CONFIG_FILE = "cloud_url_config.txt";
// URL por defecto para la palabra secreta en la nube.
// CAMBIA ESTO por tu propia URL de Pastebin RAW o GitHub Gist RAW!
const std::string DEFAULT_CLOUD_URL = "https://gist.githubusercontent.com/tu_usuario/tu_gist_id/raw/tu_archivo_clave.txt"; 

// --- Funciones de Utilidad ---

// Limpia el buffer de entrada de la consola.
void clearInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Convierte una cadena a minúsculas.
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Lee una cadena desde un archivo.
std::string readStringFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        std::getline(file, line);
        file.close();
        // Eliminar espacios en blanco al inicio y al final de la línea
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
        return line;
    }
    return ""; // Retorna cadena vacía si el archivo no se pudo abrir o no existe
}

// Escribe una cadena en un archivo.
void writeStringToFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << content;
        file.close();
    } else {
        std::cerr << "Error: No se pudo escribir en el archivo '" << filename << "'." << std::endl;
    }
}

// Descarga contenido de una URL usando curl o PowerShell (para Windows).
// Retorna el contenido como string o vacío si falla.
std::string downloadString(const std::string& url) {
    std::string command;
    #ifdef _WIN32 // Para sistemas Windows
        // Intenta usar curl.exe si está en el PATH
        command = "curl.exe -s -L \"" + url + "\"";
        // Alternativa con PowerShell si curl.exe no está disponible
        // command = "powershell -command \"(New-Object System.Net.WebClient).DownloadString('" + url + "')\"";
    #else // Para sistemas tipo Unix (Linux, macOS)
        command = "curl -s -L \"" + url + "\"";
    #endif

    std::string result = "";
    std::cout << "Intentando descargar de la nube: " << url << std::endl;
    
    // Usar _popen para capturar la salida del comando.
    // Usar "2>NUL" o "2>/dev/null" para suprimir los errores de curl a la consola.
    FILE* pipe = _popen((command + " 2>NUL").c_str(), "r"); // Para Windows, usa 2>NUL
    // FILE* pipe = popen((command + " 2>/dev/null").c_str(), "r"); // Para Unix/Linux
    if (!pipe) {
        std::cerr << "Error: No se pudo ejecutar el comando de descarga." << std::endl;
        return "";
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe); // Para Windows
    // pclose(pipe); // Para Unix/Linux

    // Eliminar saltos de línea y espacios extra del resultado
    result.erase(0, result.find_first_not_of(" \t\n\r\f\v"));
    result.erase(result.find_last_not_of(" \t\n\r\f\v") + 1);
    
    return result;
}

// Intenta obtener la palabra secreta de la nube.
// Retorna la palabra si es exitosa, o una cadena vacía si falla (e.g., sin internet).
std::string getCloudWord(const std::string& url) {
    std::string word = downloadString(url);
    if (word.empty()) {
        std::cerr << "Advertencia: No se pudo obtener la palabra de la nube. Posiblemente sin conexión a internet o URL incorrecta/vacía.\n";
        return "";
    }
    return word;
}

// Lee la URL de la nube desde un archivo de configuración local.
// Si el archivo no existe o está vacío, usa la URL por defecto y la guarda.
std::string readCloudUrlConfig() {
    std::string url = readStringFromFile(CLOUD_URL_CONFIG_FILE);
    if (url.empty() || url == "\n" || url == "\r\n") { // Considerar líneas vacías
        std::cout << "Archivo de configuracion de URL de la nube no encontrado o vacio. Usando URL por defecto.\n";
        url = DEFAULT_CLOUD_URL;
        writeStringToFile(CLOUD_URL_CONFIG_FILE, url);
    }
    return url;
}

// Escribe una nueva URL en el archivo de configuración de la nube.
void updateCloudUrlConfig(const std::string& new_url) {
    writeStringToFile(CLOUD_URL_CONFIG_FILE, new_url);
    std::cout << "URL de la nube actualizada en '" << CLOUD_URL_CONFIG_FILE << "'.\n";
}

// --- Lógica del Juego ---
void runGuessingGame(const std::string& secret_word) {
    if (secret_word.empty()) {
        std::cout << "No hay una palabra secreta para adivinar. Asegurate de que la palabra local exista o que la nube sea accesible.\n";
        return;
    }

    std::cout << "\n--- Juego de Adivinar la Palabra ---\n";
    std::cout << "La palabra secreta tiene " << secret_word.length() << " letras.\n";

    std::string guess;
    bool guessed_correctly = false;

    while (!guessed_correctly) {
        std::cout << "Tu intento: ";
        std::getline(std::cin, guess);
        guess = toLower(guess); // Convertir intento a minúsculas

        if (guess == toLower(secret_word)) { // Comparar con la palabra secreta en minúsculas
            std::cout << "¡Felicidades! Has adivinado la palabra correctamente: '" << secret_word << "'\n";
            guessed_correctly = true;
        } else {
            std::cout << "Incorrecto. Intenta de nuevo.\n";
        }
    }
}

int main() {
    std::cout << "Iniciando la aplicacion Adivinar la Palabra...\n";

    std::string current_cloud_url = readCloudUrlConfig();
    std::string target_secret_word;
    std::string local_secret_word = readStringFromFile(LOCAL_WORD_FILE);
    bool internet_available = false;

    // Intento de sincronización con la nube
    std::string cloud_word = getCloudWord(current_cloud_url);
    if (!cloud_word.empty()) {
        internet_available = true;
        std::cout << "Conectado a la nube. Palabra en la nube: '" << cloud_word << "'\n";

        if (local_secret_word.empty()) {
            std::cout << "Primera ejecucion o palabra local no encontrada. Guardando palabra de la nube localmente.\n";
            target_secret_word = cloud_word;
            writeStringToFile(LOCAL_WORD_FILE, target_secret_word);
        } else if (toLower(local_secret_word) != toLower(cloud_word)) {
            std::cout << "La palabra en la nube ha cambiado. Actualizando palabra local de '" 
                      << local_secret_word << "' a '" << cloud_word << "'.\n";
            target_secret_word = cloud_word;
            writeStringToFile(LOCAL_WORD_FILE, target_secret_word);
        } else {
            std::cout << "La palabra local esta sincronizada con la nube: '" << local_secret_word << "'\n";
            target_secret_word = local_secret_word;
        }
    } else {
        internet_available = false;
        std::cerr << "No se pudo conectar a la nube. Usando la palabra local si existe.\n";
        if (local_secret_word.empty()) {
            std::cerr << "Error: No hay conexion a internet y no se encontro ninguna palabra local. "
                      << "Por favor, establece una palabra en '" << LOCAL_WORD_FILE << "' manualmente "
                      << "o conecta a internet para descargarla desde la URL configurada.\n";
            std::cout << "URL de la nube configurada: " << current_cloud_url << "\n";
            std::cout << "Presiona cualquier tecla para salir..." << std::endl;
            std::cin.get();
            return EXIT_FAILURE;
        } else {
            std::cout << "Usando palabra local: '" << local_secret_word << "'\n";
            target_secret_word = local_secret_word;
        }
    }

    int choice;
    do {
        std::cout << "\n--- Menu Adivinar la Palabra ---\n";
        std::cout << "1. Jugar\n";
        std::cout << "2. Mostrar palabra secreta (solo para depuracion)\n";
        std::cout << "3. Cambiar URL de la palabra en la nube\n";
        std::cout << "0. Salir\n";
        std::cout << "Elige una opcion: ";
        
        std::cin >> choice;
        clearInputBuffer();

        switch (choice) {
            case 1:
                runGuessingGame(target_secret_word);
                break;
            case 2:
                std::cout << "La palabra secreta actual es: '" << target_secret_word << "'\n";
                if (!internet_available) {
                    std::cout << "Nota: Esta palabra se cargo localmente debido a la falta de conexion a internet.\n";
                }
                break;
            case 3: {
                std::string new_url;
                std::cout << "Ingresa la nueva URL RAW de tu palabra secreta en la nube (Pastebin o GitHub Gist): ";
                std::getline(std::cin, new_url);
                updateCloudUrlConfig(new_url);
                current_cloud_url = new_url; // Actualiza la URL en memoria
                std::cout << "Por favor, reinicia la aplicacion para que la nueva URL de la nube tenga efecto.\n";
                // Forzar salida para reiniciar y resincronizar
                choice = 0; 
                break;
            }
            case 0:
                std::cout << "Saliendo del juego. ¡Hasta luego!\n";
                break;
            default:
                std::cout << "Opcion invalida. Por favor, elige una opcion valida.\n";
                break;
        }
    } while (choice != 0);

    std::cout << "\nPresiona cualquier tecla para cerrar esta ventana..." << std::endl;
    std::cin.get(); 
    
    return EXIT_SUCCESS;
}
