#include "server.h"
#include <iostream>
#include <string>

// Основная функция main
int main(int argc, char *argv[]) {
    // Объявляем переменные перед их использованием
   	int port = 33333; 
    std::string file_name = "/etc/vcalc.conf"; 
    std::string file_error = "/var/log/vcalc.log";
    std::string error;

    // Обрабатываем параметры командной строки
    int result = handle_arguments(argc, argv, file_name, port, file_error);
    if (result != 0) {
        return result;
    }

    Error errors;
    if(errors.er(file_name, file_error)==12) {
        std::cout<<"Ошибка открытия файла"<<std::endl;
        return 1;
    }

    Server server(errors);
    Authorized authorized(errors);
    Calculator calculator(errors);
    int s = server.self_addr(error, file_error, port);
    while(true) {
        int work_sock = server.client_addr(s, error, file_error);
        authorized.authorized(work_sock, file_name, file_error);
        calculator.calc(work_sock);
    }
    return 0;
}
