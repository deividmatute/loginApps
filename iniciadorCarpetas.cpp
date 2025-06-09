#include <iostream>
#include <string>
#include <filesystem> // C++17 for directory and file operations
#include <limits>     // For std::numeric_limits

// Namespace alias for convenience
namespace fs = std::filesystem;

// Function to prepare a target directory: removes all its contents if it exists, then creates it.
// This is crucial for ensuring the destination is empty before copying new content.
void prepareAndCleanDirectory(const fs::path& dir_path) {
    if (fs::exists(dir_path)) {
        if (fs::is_directory(dir_path)) {
            std::cout << "Limpiando el contenido existente en: " << dir_path.string() << std::endl;
            try {
                // Iterate and remove all entries within the directory
                for (const auto& entry : fs::directory_iterator(dir_path)) {
                    fs::remove_all(entry.path()); // Remove files and subdirectories recursively
                }
                std::cout << "Contenido de '" << dir_path.string() << "' eliminado exitosamente." << std::endl;
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error al limpiar el directorio '" << dir_path.string() << "': " << e.what() << std::endl;
                // It's critical to exit if we can't clean as it violates the requirement of a clean state
                exit(EXIT_FAILURE); 
            }
        } else {
            std::cerr << "Error: La ruta '" << dir_path.string() << "' existe pero no es un directorio. No se puede limpiar." << std::endl;
            exit(EXIT_FAILURE);
        }
    } else {
        std::cout << "El directorio '" << dir_path.string() << "' no existe. Se creara." << std::endl;
    }
    
    // Ensure the directory exists after cleaning or if it didn't exist
    try {
        fs::create_directories(dir_path);
        std::cout << "Directorio '" << dir_path.string() << "' preparado." << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al crear el directorio '" << dir_path.string() << "': " << e.what() << std::endl;
        exit(EXIT_FAILURE); // Critical error, cannot proceed
    }
}

// Function to copy a file from source to destination, overwriting if needed.
// Returns true on success, false on failure.
bool copyFile(const fs::path& source_path, const fs::path& destination_path) {
    if (!fs::exists(source_path)) {
        std::cerr << "Error: El archivo origen '" << source_path.string() << "' no existe. No se puede copiar." << std::endl;
        return false;
    }
    try {
        // Explicitly remove the destination file if it exists before copying
        if (fs::exists(destination_path)) {
            fs::remove(destination_path);
            std::cout << "  Eliminando archivo de destino existente: '" << destination_path.string() << "'" << std::endl;
        }
        // Copy with overwrite_existing to ensure destination is updated/emptied effectively for a single file
        fs::copy(source_path, destination_path, fs::copy_options::overwrite_existing);
        std::cout << "✅ Copiado: '" << source_path.string() << "' a '" << destination_path.string() << "'" << std::endl;
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al copiar el archivo '" << source_path.string() << "' a '" << destination_path.string() << "': " << e.what() << std::endl;
        return false;
    }
}

// Function to copy contents of a source directory to a prepared (cleaned) destination directory.
// This function specifically iterates over files and subdirectories in the source directory and copies them.
// It assumes the destination directory has already been cleaned and created by prepareAndCleanDirectory.
// Returns true on success, false on failure.
bool copyDirectoryContents(const fs::path& source_dir, const fs::path& dest_dir) {
    if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
        std::cerr << "Error: El directorio origen '" << source_dir.string() << "' no existe o no es un directorio. No se pueden copiar sus contenidos." << std::endl;
        return false;
    }

    try {
        bool success = true;
        std::cout << "  Copiando contenido de: '" << source_dir.string() << "'..." << std::endl;
        for (const auto& entry : fs::directory_iterator(source_dir)) {
            fs::path current_dest_path = dest_dir / entry.path().filename(); // Path for the item INSIDE dest_dir
            if (fs::is_regular_file(entry.status())) {
                fs::copy(entry.path(), current_dest_path, fs::copy_options::overwrite_existing);
                std::cout << "    ✅ Copiado archivo: '" << entry.path().filename().string() << "'" << std::endl;
            } else if (fs::is_directory(entry.status())) {
                // If it's a directory, recursively copy its contents into the destination
                // This will create the subdirectory inside dest_dir and copy all its contents.
                fs::copy(entry.path(), current_dest_path, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
                std::cout << "    ✅ Copiado directorio recursivamente: '" << entry.path().filename().string() << "'" << std::endl;
            }
        }
        return success;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al copiar contenidos del directorio '" << source_dir.string() << "' a '" << dest_dir.string() << "': " << e.what() << std::endl;
        return false;
    }
}


int main(int argc, char* argv[]) { // Modified main to accept command-line arguments
    // Check if a video project folder name was provided as an argument
    if (argc < 2) {
        std::cerr << "Error: Se requiere el nombre de la carpeta del proyecto de video como argumento.\n";
        std::cerr << "Uso: " << argv[0] << " <nombre_carpeta_proyecto_video>\n";
        std::cerr << "Ejemplo: " << argv[0] << " Vid0001\n";
        return EXIT_FAILURE;
    }

    std::string video_project_folder_name = argv[1]; // Get the video folder name from the first argument

    std::cout << "Iniciando la organizacion de archivos y carpetas para el proyecto: '" << video_project_folder_name << "'\n";
    std::cout << "Se asume que este programa se ejecuta desde la carpeta 'MiApp/Librerias/'.\n";
    std::cout << "Los archivos fuente se buscaran en 'MiApp/Aplicacion/Datos de Videos/" << video_project_folder_name << "/'.\n\n";

    // Define base paths relative to the assumed working directory (MiApp/Librerias/)
    fs::path app_base_dir = "../Aplicacion";    // To go from Librerias/ up to MiApp/ and then into Aplicacion/
    fs::path librerias_base_dir = ".";          // The current directory, MiApp/Librerias/

    // Source paths within the specific video project folder (relative to MiApp/Aplicacion/)
    fs::path source_video_data_base_dir = app_base_dir / "Datos de Videos" / video_project_folder_name; // NEW BASE SOURCE
    fs::path source_audios_sin_nombres_dir = source_video_data_base_dir / "Audios Sin Nombres";
    fs::path source_personajes_dir = source_video_data_base_dir / "Personajes";
    fs::path source_excel_txt = source_video_data_base_dir / "Excel.txt";
    fs::path source_subfrases_txt = source_video_data_base_dir / "SubFrases.txt"; // CORRECTED TYPO HERE

    // Destination paths within 'Librerias' (relative to MiApp/Librerias/ or '.')
    fs::path dest_audios_sin_nombres_dir = librerias_base_dir / "Audios_Sin_Nombres";
    fs::path dest_personajes_dir = librerias_base_dir / "personajes";
    fs::path dest_excel_txt = librerias_base_dir / "Excel.txt";
    fs::path dest_cantidad_sub_frases_txt = librerias_base_dir / "Cantidad_Sub_Frases.txt";

    bool all_operations_successful = true; // Flag to track overall success

    // --- 1. Mover contenido de 'Audios Sin Nombres' ---
    std::cout << "\n--- Procesando Audios Sin Nombres ---\n";
    prepareAndCleanDirectory(dest_audios_sin_nombres_dir); // Clean and prepare destination
    if (!copyDirectoryContents(source_audios_sin_nombres_dir, dest_audios_sin_nombres_dir)) {
        all_operations_successful = false;
        std::cerr << "❌ Fallo al copiar contenidos de 'Audios Sin Nombres'." << std::endl;
    }

    // --- 2. Mover contenido de 'Personajes' ---
    std::cout << "\n--- Procesando Personajes ---\n";
    prepareAndCleanDirectory(dest_personajes_dir); // Clean and prepare destination
    if (!copyDirectoryContents(source_personajes_dir, dest_personajes_dir)) {
        all_operations_successful = false;
        std::cerr << "❌ Fallo al copiar contenidos de 'Personajes'." << std::endl;
    }
    
    // --- 3. Mover 'Excel.txt' a 'Librerias/Excel.txt' ---
    std::cout << "\n--- Procesando Excel.txt ---\n";
    if (!copyFile(source_excel_txt, dest_excel_txt)) {
        all_operations_successful = false;
        std::cerr << "❌ Fallo al copiar 'Excel.txt'." << std::endl;
    }

    // --- 4. Mover 'SubFrases.txt' a 'Librerias/Cantidad_Sub_Frases.txt' ---
    std::cout << "\n--- Procesando SubFrases.txt (se renombrara a Cantidad_Sub_Frases.txt) ---\n";
    if (!copyFile(source_subfrases_txt, dest_cantidad_sub_frases_txt)) {
        all_operations_successful = false;
        std::cerr << "❌ Fallo al copiar 'SubFrases.txt'." << std::endl;
    }

    if (all_operations_successful) {
        std::cout << "\n🎉 ¡Todos los archivos y carpetas han sido organizados y movidos exitosamente para el proyecto '" << video_project_folder_name << "'!\n";
    } else {
        std::cerr << "\n⚠️ Se encontraron errores durante la organizacion de archivos para el proyecto '" << video_project_folder_name << "'. Revisa los mensajes anteriores.\n";
    }

    // No esperar tecla aquí, ya que será llamado por otro exe
    // std::cout << "\nPresiona cualquier tecla para cerrar esta ventana..." << std::endl;
    // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
    // std::cin.get(); 

    return all_operations_successful ? EXIT_SUCCESS : EXIT_FAILURE;
}
