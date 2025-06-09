#include <iostream>  // Para entrada/salida de consola (std::cout, std::cin)
#include <string>    // Para manipulación de cadenas (std::string, std::getline)
#include <limits>    // Para std::numeric_limits (usado para limpiar el buffer de entrada)
#include <cctype>    // Para std::tolower (usado para hacer las respuestas 's/n' insensibles a mayúsculas/minúsculas)
#include <cstdlib>   // Para std::system (usado para ejecutar comandos externos)
#include <filesystem> // Para listar directorios
#include <vector>    // Required for std::vector
#include <algorithm> // Required for std::sort

namespace fs = std::filesystem; // Alias para std::filesystem

// Función para limpiar el buffer de entrada de la consola.
// Esto es importante para evitar que caracteres sobrantes (como el Enter después de un número)
// interfieran con lecturas de entrada posteriores.
void clearInputBuffer() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Función para obtener una respuesta de 'sí' ('s') o 'no' ('n') del usuario.
// Se encarga de validar la entrada y pedirla de nuevo si no es válida.
char getYesNoResponse(const std::string& prompt) {
    char response;
    while (true) {
        std::cout << prompt; // Muestra el mensaje de la pregunta al usuario
        std::cin >> response; // Lee la respuesta del usuario
        clearInputBuffer();   // Limpia el buffer de entrada para la siguiente lectura

        // Convierte la respuesta a minúsculas para una comparación insensible a mayúsculas/minúsculas
        response = std::tolower(response); 
        
        // Si la respuesta es 's' o 'n', la entrada es válida y salimos del bucle
        if (response == 's' || response == 'n') {
            break;
        } else {
            // Si la respuesta no es 's' ni 'n', informa al usuario y pide de nuevo
            std::cout << "Respuesta invalida. Por favor, ingresa 's' (si) o 'n' (no).\n";
        }
    }
    return response; // Devuelve la respuesta validada
}

// Función para ejecutar un programa externo y verificar su código de salida.
// Retorna 'true' si el programa se ejecutó sin errores, 'false' en caso contrario.
// Ahora acepta un argumento opcional para el programa y encierra el nombre en comillas.
bool executeProgram(const std::string& program_name, const std::string& args = "") {
    // Agrega "cmd /c " para forzar la ejecución a través del intérprete de comandos de Windows.
    // Esto ayuda a resolver la ruta de los ejecutables en el directorio actual.
    std::string command = "cmd /c \"" + program_name + "\""; 
    if (!args.empty()) {
        command += " " + args; // Añade los argumentos si existen
    }
    std::cout << "Ejecutando: " << command << "...\n"; // Informa al usuario qué comando se está ejecutando
    int result = std::system(command.c_str()); // Ejecuta el programa y obtiene el código de salida

    // Un código de salida distinto de cero generalmente indica un error
    if (result != 0) {
        std::cerr << "Error al ejecutar " << command << ". Codigo de salida: " << result << std::endl;
        return false; // Indica que la ejecución falló
    }
    std::cout << "✅ " << program_name << " completado exitosamente.\n"; // Mensaje de éxito
    return true; // Indica que la ejecución fue exitosa
}

int main() {
    int choice; // Variable para almacenar la opción del menú elegida por el usuario

    // Define la ruta a la carpeta "Datos de Videos"
    // ASUMIMOS que App.exe se ejecuta desde MiApp/Librerias/
    // La carpeta "Datos de Videos" está en MiApp/Aplicacion/
    // Por lo tanto, para llegar a "Datos de Videos" desde "Librerias", debemos ir un nivel arriba (..) y luego a "Aplicacion/Datos de Videos"
    const fs::path datos_de_videos_path = "../Aplicacion/Datos de Videos"; 

    // Bucle principal del menú: se repite hasta que el usuario elija '0' (Salir)
    do {
        // Muestra las opciones del menú al usuario
        std::cout << "\n--- Menu Principal ---\n";
        std::cout << "1. Crear Partes Video Dialogo (procesara todas las carpetas en 'Datos de Videos')\n";
        std::cout << "0. Salir\n";
        std::cout << "Elige una opcion: ";
        
        std::cin >> choice;     // Lee la opción numérica
        clearInputBuffer();     // Limpia el buffer de entrada

        // Estructura switch para manejar las diferentes opciones del menú
        switch (choice) {
            case 1: { // Opción "Crear Partes Video Dialogo"
                std::cout << "\n--- Creacion de Partes de Video Dialogo ---\n";
                char response; // Variable para almacenar las respuestas 's/n' del usuario

                // Único requisito: Verificar si el usuario ha preparado todos los datos en la carpeta "Datos de Videos"
                response = getYesNoResponse("¿Ya te aseguraste de que cada carpeta de proyecto dentro de 'MiApp/Aplicacion/Datos de Videos/' "
                                             "contiene todos los archivos necesarios (texto, imagenes y audios) "
                                             "para cada video? (s/n): ");
                if (response == 'n') {
                    std::cout << "❌ Por favor, asegurate de llenar las subcarpetas dentro de 'Datos de Videos' con todos los archivos requeridos antes de continuar.\n";
                    break; 
                }
                
                std::cout << "\nIniciando el procesamiento de los proyectos de video en '" << datos_de_videos_path << "'...\n";
                
                std::vector<fs::path> video_project_folders;
                try {
                    if (fs::exists(datos_de_videos_path) && fs::is_directory(datos_de_videos_path)) {
                        for (const auto& entry : fs::directory_iterator(datos_de_videos_path)) {
                            if (fs::is_directory(entry.status())) {
                                video_project_folders.push_back(entry.path());
                            }
                        }
                        // Ordenar las carpetas por nombre para un procesamiento predecible (opcional)
                        std::sort(video_project_folders.begin(), video_project_folders.end());
                    } else {
                        // Mensaje de error ajustado para reflejar la ruta corregida
                        std::cerr << "Error: La carpeta '" << datos_de_videos_path.string() << "' no existe o no es un directorio.\n"
                                  << "Asegurate de que la ruta 'MiApp/Aplicacion/Datos de Videos/' exista y contenga tus carpetas de proyecto.\n";
                        break;
                    }
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Error al listar directorios en '" << datos_de_videos_path.string() << "': " << e.what() << std::endl;
                    break;
                }

                if (video_project_folders.empty()) {
                    std::cout << "No se encontraron carpetas de proyectos de video en '" << datos_de_videos_path.string() << "'. No se generara ningun video.\n";
                    break;
                }

                bool all_videos_successful = true; // Bandera global para el éxito de todos los videos
                for (const auto& project_folder : video_project_folders) {
                    std::string project_name = project_folder.filename().string();
                    std::cout << "\n--- Procesando Proyecto de Video: '" << project_name << "' ---\n";
                    
                    bool current_video_successful = true; // Bandera para el éxito del video actual

                    // 1. Ejecutar iniciadorCarpetas.exe con el nombre de la carpeta del proyecto
                    std::cout << "Preparando la estructura de archivos para el proyecto '" << project_name << "'...\n";
                    // App.exe e iniciadorCarpetas.exe están en la misma carpeta (MiApp/Librerias/)
                    // Por lo tanto, la ruta relativa es "iniciadorCarpetas.exe" (sin "./")
                    if (!executeProgram("iniciadorCarpetas.exe", project_name)) { 
                        std::cerr << "❌ Error: 'iniciadorCarpetas.exe' fallo para el proyecto '" << project_name << "'. No se puede continuar con la generacion de este video.\n";
                        current_video_successful = false; // Marcar la secuencia como fallida para este video
                    }
                    // Mover este mensaje de éxito DENTRO del bloque 'if' para que solo se imprima si el programa fue exitoso
                    if (current_video_successful) { 
                        std::cout << "✅ Estructura de archivos preparada para '" << project_name << "'.\n";
                        // Secuencia de ejecución de los programas, deteniéndose si alguno falla
                        // Todos los .exe están en la misma carpeta (MiApp/Librerias/)
                        if (current_video_successful) current_video_successful = executeProgram("obtenerFragmentos.exe");
                        if (current_video_successful) current_video_successful = executeProgram("imagenes.exe");
                        if (current_video_successful) current_video_successful = executeProgram("generar_nombres_audios.exe");
                        if (current_video_successful) current_video_successful = executeProgram("normalizar_audios.exe");
                        if (current_video_successful) current_video_successful = executeProgram("generar_audios_main.exe");
                        if (current_video_successful) current_video_successful = executeProgram("generar_videos.exe", project_name); 
                    }
                    
                    // Muestra un mensaje final sobre el resultado de este video específico
                    if (current_video_successful) {
                        std::cout << "\n🎉 Video del proyecto '" << project_name << "' creado exitosamente!\n";
                    } else {
                        std::cerr << "\n❌ La generacion del video para el proyecto '" << project_name << "' fallo. Revisa los mensajes de error individuales.\n";
                        all_videos_successful = false; // Marcar que al menos un video falló en el proceso general
                    }
                    std::cout << "--- Fin del procesamiento para el proyecto: '" << project_name << "' ---\n";
                }

                // Mensaje final que resume el éxito de toda la tanda de videos
                if (all_videos_successful) {
                    std::cout << "\n\n🎉 ¡Todos los videos de los proyectos han sido creados exitosamente!\n";
                } else {
                    std::cerr << "\n\n❌ Uno o mas proyectos de video fallaron. Revisa los mensajes de error individuales.\n";
                }
                break; // Sale del switch
            }
            case 0: // Opción "Salir"
                std::cout << "Saliendo del programa. ¡Hasta luego!\n";
                break; // Sale del switch, y el bucle 'do-while' terminará
            default: // Cualquier otra entrada no válida
                std::cout << "Opcion invalida. Por favor, selecciona 1 o 0.\n";
                break; // Sale del switch y regresa al menú principal
        }
    } while (choice != 0); // Continúa el bucle mientras la elección no sea '0'

    // Pausa la consola al final para que el usuario pueda leer los mensajes antes de que se cierre
    std::cout << "\nPresiona cualquier tecla para cerrar esta ventana..." << std::endl;
    // Se usa std::cin.get() dos veces para manejar posibles newlines residuales en el buffer de entrada
    std::cin.get(); 
    std::cin.get(); 
    
    return 0; // El programa finaliza con éxito
}
