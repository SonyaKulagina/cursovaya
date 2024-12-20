#include "server.h"

int handle_arguments(int argc, char *argv[], std::string& file_name, int& port, std::string& file_error) {
   
    file_name = "";
    port = 0;
    file_error = "";

    const struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"file", required_argument, 0, 'f'},
        {"port", required_argument, 0, 'p'},
        {"error", required_argument, 0, 'e'},
        {0, 0, 0, 0}
    };

    int option;
    int option_index = 0;

    // Обрабатываем аргументы командной строки
    while ((option = getopt_long(argc, argv, "hf:p:e:", long_options, &option_index)) != -1) {
        switch (option) {
            case 'h':
                std::cout << "Калькулятор" << std::endl;
                std::cout << "-h --help Помощь" << std::endl;
                std::cout << "-f --file Название файла" << std::endl;
                std::cout << "-p --port Порт" << std::endl;
                std::cout << "-e --error Файл ошибок" << std::endl;
                return EXIT_SUCCESS;
            case 'f': {
                file_name = std::string(optarg); // Обновляем file_name
            }
                break;
            case 'p': {
                try {
                    port = std::stoi(std::string(optarg)); // Преобразуем порт в число
                    if (port < 0 || port > 65535) {
                        std::cerr << "Ошибка: порт должен быть в диапазоне от 0 до 65535." << std::endl;
                        return EXIT_FAILURE;
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Ошибка: неверный формат порта." << std::endl;
                    return EXIT_FAILURE;
                }
            }
                break;
            case 'e': {
                file_error = std::string(optarg); // Обновляем file_error
            }
                break;
            case '?':
                std::cerr << "Неверно введен параметр. Используйте -h для помощи." << std::endl;
                return EXIT_FAILURE;
            default:
                std::cerr << "Неизвестный параметр. Используйте -h для помощи." << std::endl;
                return EXIT_FAILURE;
        }
    }

    // Устанавливаем значения по умолчанию, если аргументы не переданы
    if (file_name.empty()) {
        file_name = "/etc/vcalc.conf";
    }
    if (port == 0) {
        port = 33333;
    }
    if (file_error.empty()) {
        file_error = "/var/log/vcalc.log";
    }

    return EXIT_SUCCESS;
}

void Authorized::msgsend(int work_sock, const std::string& mess) {
    // Используем std::vector для автоматического управления памятью
    std::vector<char> buffer(mess.begin(), mess.end());

    // Отправляем сообщение через сокет
    send(work_sock, buffer.data(), buffer.size(), 0);
}

std::string Authorized::SHA(const std::string& sah) {
    CryptoPP::SHA1 hash;  // Используем SHA-1
    std::string digest;
    // Используем StringSource для хеширования с помощью HashFilter и HexEncoder
    CryptoPP::StringSource(sah, true,
        new CryptoPP::HashFilter(hash,
            new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));  // Хеш хранится в digest

    return digest;  // Возвращаем хеш
}
 
std::string Authorized::salt_generator(const std::size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::string salt;
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);

    for (std::size_t i = 0; i < length; i++) {
        salt += charset[dist(rng)];
    }

    return salt;
}

Error::Error()
{

}
 
void Error::errors(std::string error, std::string name){
    std::ofstream file;
    file.open(name, std::ios::app);
    if(file.is_open()){
        time_t seconds = time(NULL);
        tm* timeinfo = localtime(&seconds);
        file<<error<<':'<<asctime(timeinfo)<<std::endl;
        std::cout << "error: " << error << std::endl;
    }
}
int Error::er(std::string file_name, std::string file_error){
        std::fstream file;
        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        try{
        file.open(file_name);
        return 1;
        }catch(const std::exception & ex){
        std::string error = "Ошибка открытия файла";
        errors(error, file_error);
        return 12;
        }
        }

void alarm_handler(int signal) {
    std::cout << "Время ожидания истекло\n";
    exit(EXIT_FAILURE);
}

int Server::self_addr(std::string& error, std::string& file_error, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Ошибка при создании сокета");
        exit(EXIT_FAILURE);
    }
    
    int on = 1;
    int rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "Ошибка установки сокета");
    }
    
    signal(SIGALRM, alarm_handler);
    alarm(240);
    
    struct timeval timeout {240, 0};
    rc = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "Ошибка установки сокета");
    }
    
    sockaddr_in self_addr;
    self_addr.sin_family = AF_INET;
    self_addr.sin_port = htons(port);
    self_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    std::cout << "Ожидание подключения клиента...\n";
    
    int b = bind(sock, reinterpret_cast<sockaddr*>(&self_addr), sizeof(self_addr));
    if (b == -1) {
        std::cout << "Ошибка привязки\n";
        error = "Ошибка";
        e_error.errors(error, file_error);
        return 1;
    }
    
    listen(sock, SOMAXCONN);
    
    return sock;
}

int Server::client_addr(int s, std::string& error, std::string& file_error) {
        // код функции client_addr
        sockaddr_in * client_addr = new sockaddr_in;
        socklen_t len = sizeof (sockaddr_in);
        
        int work_sock = accept(s, (sockaddr*)(client_addr), &len);
        if(work_sock == -1) {
            std::cout << "Ошибка\n";
            error = "Ошибка";
            e_error.errors(error, file_error);
            return 1;
        }
        else {
            //Успешное подключение к серверу
            std::cout << "Клиент успешно подключился!\n";
            return work_sock;
        }
    }

int Authorized::authorized(int work_sock, std::string file_name, std::string file_error)
{
    std::string ok = "OK";
    std::string err = "ERR";
    std::string error;
    char msg[255];

    //АВТОРИЗАЦИЯ
    recv(work_sock, &msg, sizeof(msg), 0);
    std::string message = msg;
    std::string login, hashq;
    std::fstream file;
    file.open(file_name);
    getline (file, login, ':');
    getline (file, hashq);

    //СВЕРКА ЛОГИНА
    if(message != login){
        msgsend(work_sock,  err);
        error = "Ошибка логина";
        e_error.errors(error, file_error);
        close(work_sock);
        return 1;
    }else{

        //РАБОТА С СОЛЬЮ
        std::string salt = salt_generator(16); // Генерируем соль длиной 16 байтов
        msgsend(work_sock,  salt);
        recv(work_sock, msg, sizeof(msg), 0);
        std::string sah = salt + hashq;
        std::string digest;
        digest = SHA(sah);

        //СВЕРКА ПАРОЛЯ
        if(digest != msg){
            std::cout << digest << std::endl;
            std::cout << msg << std::endl;
            msgsend(work_sock,  err);
            error = "Ошибка пароля";
            e_error.errors(error, file_error);
            close(work_sock);
            return 1;
        }else{
            msgsend(work_sock,  ok);
        }
    }
    return 1;
}

int Calculator::calc(int work_sock)
{
    int Quantity;
    int Length;
    float Vector_numbers;

    // Получение количества векторов
    recv(work_sock, &Quantity, sizeof(Quantity), 0);

    for (int j = 0; j < Quantity; j++) {
        // Получение длины каждого вектора
        recv(work_sock, &Length, sizeof(Length), 0);

        float Product = 1.0; // Инициализация произведения

        for (int i = 0; i < Length; i++) {
            // Получение каждого элемента вектора
            recv(work_sock, &Vector_numbers, sizeof(Vector_numbers), 0);
            Product *= Vector_numbers; // Умножаем на каждый элемент
        }

        // Передаем результат произведения (Тип float)
        send(work_sock, &Product, sizeof(Product), 0);
    }

    std::cout << "Завершение работы программы" << std::endl;
    close(work_sock);
    return 1; // Возвращаем 1 при завершении
}
