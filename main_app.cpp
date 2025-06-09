#include <iostream>
#include <string>
#include <limits> // Para numeric_limits

// Funcion para ejecutar un comando del sistema
// En Windows, se utiliza directamente el nombre del ejecutable.
// En otros sistemas, se podria necesitar './ejecutable' o la ruta completa.
void execute_program(const std::string& program_name) {
    std::cout << "\nEjecutando: " << program_name << "...\n";
    // system() ejecuta el comando en una nueva shell.
    // Esto significa que el control vuelve a este programa una vez que el ejecutado termina.
    std::system(program_name.c_str());
    std::cout << "\n" << program_name << " ha terminado su ejecucion.\n";
}

int main() {
    int choice;

    do {
        // Limpiar la pantalla de la consola para un menu mas limpio (opcional)
        // Para Windows: system("cls");
        // Para Linux/macOS: system("clear");
        // Nota: El uso de system() no es portatil y puede tener implicaciones de seguridad.
        // Para aplicaciones de produccion, se preferirian metodos mas robustos.

        std::cout << "--- Menu Principal de Aplicaciones ---\n";
        std::cout << "1. Renombrar Audios (Ejecuta: generar_nombres_audios.exe)\n";
        std::cout << "2. Crear Audios Main (Ejecuta: generar_audios_main.exe)\n";
        std::cout << "0. Salir\n";
        std::cout << "Ingrese su opcion: ";

        // Validar la entrada del usuario
        while (!(std::cin >> choice)) {
            std::cout << "Entrada invalida. Por favor, ingrese un numero: ";
            std::cin.clear(); // Limpiar el estado de error
            // Descartar la entrada incorrecta hasta el final de la linea
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        // Descartar el resto de la linea (el caracter de nueva linea despues del numero)
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1:
                execute_program("generar_nombres_audios.exe");
                break;
            case 2:
                execute_program("generar_audios_main.exe");
                break;
            case 0:
                std::cout << "\nSaliendo de la aplicacion principal. Hasta luego!\n";
                break;
            default:
                std::cout << "\nOpcion no valida. Por favor, intente de nuevo.\n";
                break;
        }

        if (choice != 0) {
            std::cout << "\nPresione ENTER para volver al menu principal...\n";
            std::cin.get(); // Esperar a que el usuario presione Enter
        }

    } while (choice != 0);

    return 0;
}
