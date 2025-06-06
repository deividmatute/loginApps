#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <chrono>

#ifdef _WIN32
#include <conio.h> // Para _getch() en Windows
#else
#include <termios.h> // Para cambiar el modo de terminal en Linux/macOS
#include <unistd.h>  // Para STDIN_FILENO
#endif

// Incluir libcurl para hacer peticiones HTTP
#include <curl/curl.h>

namespace fs = std::filesystem;

// Define la ruta base para los audios, se asume que 'Audios' esta en el mismo nivel que el ejecutable
const fs::path RUTA_BASE_AUDIOS_ABSOLUTA = "Audios"; // O ajusta a la ruta real si no es relativa, ej: "C:/TuProyecto/Audios"

// URL donde se almacenara la contrasena en texto plano
// ADVERTENCIA: ESTO NO ES SEGURO PARA APLICACIONES REALES, YA QUE EXPONE LA CONTRASENA EN LA RED Y EN EL SERVIDOR.
// DEBES COLOCAR AQUI LA URL DIRECTA DE UN ARCHIVO DE TEXTO QUE CONTENGA SOLO LA CONTRASENA.
//
// EJEMPLO de URL CORRECTA para GitHub (usando 'raw.githubusercontent.com'):
// const std::string PASSWORD_URL = "https://raw.githubusercontent.com/DeividMGZ/loginApps/main/password.txt";
//
// Asegurate de que esta URL, al abrirla en el navegador, SOLO muestre la contrasena (ej. "1") y nada mas.
const std::string PASSWORD_URL = "https://raw.githubusercontent.com/DeividMG2/loginApps/main/password.txt"; // <--- IMPORTANTE: CAMBIA ESTA URL!

// Funcion para ejecutar un comando del sistema
// Devuelve el codigo de salida del comando
int execute_command(const std::string& command) {
    // std::cout << "Ejecutando: " << command << "\n"; // Descomentar para depurar el comando de FFmpeg
    int result = std::system(command.c_str());
    return result;
}

// Funcion para normalizar un archivo de audio usando FFmpeg loudnorm
// Ahora usa un archivo temporal para sobrescribir
bool normalize_audio_with_ffmpeg(const fs::path& audio_path, double target_lufs) {
    fs::path temp_audio_path = audio_path;
    temp_audio_path.replace_extension(".temp.mp3"); // Nombre temporal con extension .temp.mp3

    // Comando para normalizar al archivo temporal
    std::string normalize_command = "ffmpeg -i ";
    normalize_command += "\"" + audio_path.string() + "\""; // Input
    normalize_command += " -af loudnorm=I=" + std::to_string(target_lufs) + ":LRA=7:TP=-2";
    normalize_command += " -ar 44100 -b:a 192k "; // Opciones de salida (sample rate, bitrate)
    normalize_command += "\"" + temp_audio_path.string() + "\" -y -loglevel quiet"; // Output y -y para sobrescribir temp si existe, -loglevel quiet para menos spam

    if (execute_command(normalize_command) != 0) {
        // Si FFmpeg falla, intentar limpiar el archivo temporal (si se creo)
        if (fs::exists(temp_audio_path)) {
            try {
                fs::remove(temp_audio_path);
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error al limpiar archivo temporal: " << temp_audio_path.string() << " -> " << e.what() << "\n";
            }
        }
        return false; // Fallo la normalizacion
    }

    // Si la normalizacion fue exitosa, reemplazar el archivo original con el temporal
    try {
        if (fs::exists(audio_path)) {
            fs::remove(audio_path); // Eliminar el original
        }
        fs::rename(temp_audio_path, audio_path); // Renombrar el temporal al nombre original
        return true; // Exito
    } catch (const fs::filesystem_error& e) {
        std::cerr << "X Error al reemplazar el archivo original con el normalizado: " << e.what() << "\n";
        // Intentar limpiar el temporal si el renombrado falla
        if (fs::exists(temp_audio_path)) {
            try {
                fs::remove(temp_audio_path);
            } catch (const fs::filesystem_error& e2) {
                std::cerr << "Error al limpiar archivo temporal despues de fallo en renombrado: " << e2.what() << "\n";
            }
        }
        return false; // Fallo el reemplazo
    }
}

// Funcion para leer la contrasena sin mostrarla en pantalla
std::string get_password_input() {
    std::string password;
#ifdef _WIN32
    char ch;
    while ((ch = _getch()) != '\r') { // Lee caracteres hasta Enter
        if (ch == '\b') { // Maneja Backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b"; // Borra el caracter del display
            }
        } else {
            password.push_back(ch);
            // No se imprime nada para ocultar la entrada
        }
    }
    std::cout << "\n"; // Nueva linea despues de Enter
#else
    // Para sistemas basados en Unix (Linux, macOS)
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt); // Guarda la configuracion de la terminal
    termios newt = oldt;
    newt.c_lflag &= ~ECHO; // Deshabilita el eco de caracteres
    tcsetattr(STDIN_FILENO, TCSANOW, &newt); // Aplica la nueva configuracion

    std::getline(std::cin, password); // Lee la contrasena

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restaura la configuracion de la terminal
#endif
    return password;
}

// Callback para almacenar la respuesta de la peticion HTTP
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

// Funcion para obtener la contrasena de una URL
std::string get_password_from_url(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT); // Inicializar libcurl
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); // Fallar si el codigo HTTP es 4xx/5xx

        // Opciones para mayor compatibilidad de SSL (deshabilita la verificacion, NO RECOMENDADO EN PROD)
        // Solo usalo si tienes problemas con certificados SSL y estas seguro de los riesgos.
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Error al obtener la contrasena de la URL: " << curl_easy_strerror(res) << std::endl;
            readBuffer = ""; // Retorna una cadena vacia en caso de error
        }
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Error al inicializar libcurl." << std::endl;
    }
    curl_global_cleanup(); // Limpiar libcurl
    // Eliminar posibles caracteres de nueva linea al final (especialmente de archivos web)
    readBuffer.erase(std::remove(readBuffer.begin(), readBuffer.end(), '\r'), readBuffer.end());
    readBuffer.erase(std::remove(readBuffer.begin(), readBuffer.end(), '\n'), readBuffer.end());
    return readBuffer;
}


int main() {
    std::cout << "Asegurate de que NINGUN archivo MP3 este abierto antes de continuar....\n";

    // --- Autenticacion de Contrasena al inicio ---
    std::string entered_password;
    int attempts = 0;
    const int MAX_ATTEMPTS = 3;

    std::string cloud_password = get_password_from_url(PASSWORD_URL);

    if (cloud_password.empty()) {
        std::cout << "\nPresiona cualquier tecla para cerrar esta ventana..." << std::endl;
        std::cin.get();
        return 1;
    }
    
    // La contrasena obtenida de la nube es la contrasena maestra
    const std::string& MASTER_PASSWORD = cloud_password; 

    while (attempts < MAX_ATTEMPTS) {
        std::cout << "\nIngrese '1' para Iniciar: ";
        entered_password = get_password_input(); // Llama a la funcion para leer la contrasena oculta

        if (entered_password == MASTER_PASSWORD) {
            break; // Sale del bucle si la contrasena es correcta
        } else {
            attempts++;
            std::cout << "Ingrese '1' para Iniciar " << (MAX_ATTEMPTS - attempts) << "\n";
        }
    }
    // --- Fin de la autenticacion ---

    std::cout << "¿Estas listo para comenzar a modificar los niveles de Volumen? (s/n): ";
    char confirmacion;
    std::cin >> confirmacion;
    if (confirmacion != 's' && confirmacion != 'S') {
        std::cout << "X Cancelado por el usuario.\n";
        return 0;
    }
    std::cin.ignore(); // Limpiar el buffer de entrada para la siguiente lectura de linea

    double target_lufs = -23.0; // Valor por defecto
    std::string lufs_input;
    std::cout << "Ingrese ENTER: ";
    std::getline(std::cin, lufs_input);
    try {
        if (!lufs_input.empty()) {
            target_lufs = std::stod(lufs_input);
        }
    } catch (const std::exception& e) {
        std::cerr << "Valor de LUFS invalido: " << e.what() << ". Usando -23.0 LUFS por defecto.\n";
    }

    // --- Definir las rutas a procesar ---
    std::vector<fs::path> folders_to_process;
    fs::path frases_english_path = RUTA_BASE_AUDIOS_ABSOLUTA / "Frases_English";
    fs::path frases_spanish_path = RUTA_BASE_AUDIOS_ABSOLUTA / "Frases_Spanish";

    if (!fs::exists(RUTA_BASE_AUDIOS_ABSOLUTA)) {
        std::cerr << "X Error: La carpeta base de audios '" << RUTA_BASE_AUDIOS_ABSOLUTA.string() << "' no existe. Asegurate de que este en la ubicacion correcta.\n";
        std::cout << "\nPresiona cualquier tecla para cerrar esta ventana..." << std::endl;
        std::cin.get();
        return 1;
    }

    folders_to_process.push_back(frases_english_path);
    folders_to_process.push_back(frases_spanish_path);

    // Determinar la cantidad de carpetas SubFrases_FraseX
    int num_english_files = 0;
    if (fs::exists(frases_english_path)) {
        for (const auto& entry : fs::directory_iterator(frases_english_path)) {
            if (entry.is_regular_file() && entry.path().extension().string() == ".mp3") {
                num_english_files++;
            }
        }
    } else {
        std::cerr << "!! Advertencia: La carpeta 'Frases_English' no existe. No se podra determinar la cantidad de carpetas 'SubFrases_FraseX'.\n";
    }

    // Anadir las carpetas SubFrases_FraseX
    for (int i = 1; i <= num_english_files; ++i) {
        folders_to_process.push_back(RUTA_BASE_AUDIOS_ABSOLUTA / ("SubFrases_Frase" + std::to_string(i)));
    }
    // ------------------------------------

    std::cout << "\nIniciando normalizacion recursiva de audios a " << target_lufs << " LUFS en las rutas predefinidas...\n";

    int total_processed_count = 0;
    int total_errors_count = 0;

    for (const auto& folder_path : folders_to_process) {
        if (!fs::exists(folder_path)) {
            std::cerr << "!! Advertencia: La carpeta '" << folder_path.string() << "' no existe. Saltando.\n";
            continue;
        }
        std::cout << "\nProcesando carpeta: " << folder_path.string() << "\n";
        
        int current_folder_processed = 0;
        int current_folder_errors = 0;

        // Recopilar todos los archivos MP3 primero para evitar problemas con iteradores modificados
        std::vector<fs::path> mp3_files_in_folder;
        // Usar recursive_directory_iterator para subcarpetas dentro de estas carpetas especificas
        for (const auto& entry : fs::recursive_directory_iterator(folder_path)) {
            if (entry.is_regular_file() && entry.path().extension().string() == ".mp3") {
                mp3_files_in_folder.push_back(entry.path());
            }
        }

        if (mp3_files_in_folder.empty()) {
            std::cout << "No se encontraron archivos MP3 en esta carpeta o sus subcarpetas. Saltando.\n";
            continue;
        }

        for (const auto& audio_filepath : mp3_files_in_folder) {
            std::cout << "  Procesando archivo: " << audio_filepath.filename().string() << "\n";
            if (normalize_audio_with_ffmpeg(audio_filepath, target_lufs)) {
                current_folder_processed++;
            } else {
                std::cerr << "X Fallo al normalizar: " << audio_filepath.filename().string() << "\n";
                current_folder_errors++;
            }
        }
        std::cout << "Resumen para '" << folder_path.filename().string() << "': " << current_folder_processed << " procesados, " << current_folder_errors << " con errores.\n";
        total_processed_count += current_folder_processed;
        total_errors_count += current_folder_errors;
    }

    if (total_processed_count == 0 && total_errors_count == 0) {
        std::cout << "\nNo se encontraron archivos MP3 en ninguna de las rutas predefinidas. Saliendo.\n";
    } else {
        std::cout << "\n--- Resumen del Proceso Global ---\n";
        std::cout << "Archivos procesados correctamente: " << total_processed_count << "\n";
        std::cout << "Archivos con errores: " << total_errors_count << "\n";
        std::cout << "Normalizacion de audios completada!\n";
    }

    std::cout << "\nPresiona cualquier tecla para cerrar esta ventana..." << std::endl;
    std::cin.get(); // Espera una tecla
    system("pause"); // Solo para Windows

    return 0;
}
