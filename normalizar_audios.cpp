#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <chrono>

namespace fs = std::filesystem;

// Define la ruta base para los audios, se asume que 'Audios' está en el mismo nivel que el ejecutable
const fs::path RUTA_BASE_AUDIOS_ABSOLUTA = "Audios"; // O ajusta a la ruta real si no es relativa, ej: "C:/TuProyecto/Audios"

// Función para ejecutar un comando del sistema
// Devuelve el código de salida del comando
int execute_command(const std::string& command) {
    // std::cout << "Ejecutando: " << command << "\n"; // Descomentar para depurar el comando de FFmpeg
    int result = std::system(command.c_str());
    return result;
}

// Función para normalizar un archivo de audio usando FFmpeg loudnorm
// Ahora usa un archivo temporal para sobrescribir
bool normalize_audio_with_ffmpeg(const fs::path& audio_path, double target_lufs) {
    fs::path temp_audio_path = audio_path;
    temp_audio_path.replace_extension(".temp.mp3"); // Nombre temporal con extensión .temp.mp3

    // Comando para normalizar al archivo temporal
    std::string normalize_command = "ffmpeg -i ";
    normalize_command += "\"" + audio_path.string() + "\""; // Input
    normalize_command += " -af loudnorm=I=" + std::to_string(target_lufs) + ":LRA=7:TP=-2";
    normalize_command += " -ar 44100 -b:a 192k "; // Opciones de salida (sample rate, bitrate)
    normalize_command += "\"" + temp_audio_path.string() + "\" -y -loglevel quiet"; // Output y -y para sobrescribir temp si existe, -loglevel quiet para menos spam

    if (execute_command(normalize_command) != 0) {
        // Si FFmpeg falla, intentar limpiar el archivo temporal (si se creó)
        if (fs::exists(temp_audio_path)) {
            try {
                fs::remove(temp_audio_path);
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error al limpiar archivo temporal: " << temp_audio_path.string() << " -> " << e.what() << "\n";
            }
        }
        return false; // Falló la normalización
    }

    // Si la normalización fue exitosa, reemplazar el archivo original con el temporal
    try {
        if (fs::exists(audio_path)) {
            fs::remove(audio_path); // Eliminar el original
        }
        fs::rename(temp_audio_path, audio_path); // Renombrar el temporal al nombre original
        return true; // Éxito
    } catch (const fs::filesystem_error& e) {
        std::cerr << "❌ Error al reemplazar el archivo original con el normalizado: " << e.what() << "\n";
        // Intentar limpiar el temporal si el renombrado falla
        if (fs::exists(temp_audio_path)) {
            try {
                fs::remove(temp_audio_path);
            } catch (const fs::filesystem_error& e2) {
                std::cerr << "Error al limpiar archivo temporal después de fallo en renombrado: " << e2.what() << "\n";
            }
        }
        return false; // Falló el reemplazo
    }
}

int main() {
  
    double target_lufs = -23.0; // Valor por defecto
  

    // --- Definir las rutas a procesar ---
    std::vector<fs::path> folders_to_process;
    fs::path frases_english_path = RUTA_BASE_AUDIOS_ABSOLUTA / "Frases_English";
    fs::path frases_spanish_path = RUTA_BASE_AUDIOS_ABSOLUTA / "Frases_Spanish";

    if (!fs::exists(RUTA_BASE_AUDIOS_ABSOLUTA)) {
        std::cerr << "❌ Error: La carpeta base de audios '" << RUTA_BASE_AUDIOS_ABSOLUTA.string() << "' no existe. Asegúrate de que esté en la ubicación correcta.\n";
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
        std::cerr << "⚠️ Advertencia: La carpeta 'Frases_English' no existe. No se podrá determinar la cantidad de carpetas 'SubFrases_FraseX'.\n";
    }

    // Añadir las carpetas SubFrases_FraseX
    for (int i = 1; i <= num_english_files; ++i) {
        folders_to_process.push_back(RUTA_BASE_AUDIOS_ABSOLUTA / ("SubFrases_Frase" + std::to_string(i)));
    }
    // ------------------------------------

    std::cout << "\nIniciando normalización recursiva de audios a " << target_lufs << " LUFS en las rutas predefinidas...\n";

    int total_processed_count = 0;
    int total_errors_count = 0;

    for (const auto& folder_path : folders_to_process) {
        if (!fs::exists(folder_path)) {
            std::cerr << "⚠️ Advertencia: La carpeta '" << folder_path.string() << "' no existe. Saltando.\n";
            continue;
        }
        std::cout << "\nProcesando carpeta: " << folder_path.string() << "\n";
        
        int current_folder_processed = 0;
        int current_folder_errors = 0;

        // Recopilar todos los archivos MP3 primero para evitar problemas con iteradores modificados
        std::vector<fs::path> mp3_files_in_folder;
        // Usar recursive_directory_iterator para subcarpetas dentro de estas carpetas específicas
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
                // std::cout << "    ✅ Normalizado y sobrescrito: " << audio_filepath.filename().string() << "\n"; // Ya lo imprime dentro de la función
                current_folder_processed++;
            } else {
                std::cerr << "    ❌ Fallo al normalizar: " << audio_filepath.filename().string() << "\n";
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
        std::cout << "¡Normalización de audios completada!\n";
    }
    return 0;
}