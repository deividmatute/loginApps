#include <iostream>
#include <string>
#include <cstdlib> // Para std::system

int main() {
    // Nombre del script Python de la GUI.
    // Asegurate de que 'lanzador_gui.py' este en la misma carpeta que este ejecutable de C++.
    const std::string python_gui_script = "gui.py";

    // Comando para ejecutar el script Python.
    // Asegurate de que 'python' este en el PATH de tu sistema.
    // Si la aplicacion Python fue empaquetada como un .exe con PyInstaller,
    // el comando seria simplemente: "./nombre_del_exe_de_python.exe"
    std::string command = "python " + python_gui_script;

    std::cout << "Lanzando la aplicacion GUI de Python: " << python_gui_script << "...\n";

    // Ejecuta el comando en el sistema.
    // system() bloquea este programa de C++ hasta que el script de Python GUI se cierre.
    int result = std::system(command.c_str());

    if (result == 0) {
        std::cout << "\nLa aplicacion GUI de Python se ha cerrado exitosamente.\n";
    } else {
        std::cerr << "\nError al lanzar o ejecutar la aplicacion GUI de Python. Codigo de salida: " << result << "\n";
        std::cerr << "Asegurate de que Python este instalado y en tu PATH, y que '" << python_gui_script << "' exista.\n";
    }

    std::cout << "Presiona cualquier tecla para cerrar esta ventana...\n";
    std::cin.ignore(); // Limpiar el buffer si no se ha limpiado antes
    std::cin.get();    // Esperar una tecla antes de cerrar la consola

    return 0;
}
