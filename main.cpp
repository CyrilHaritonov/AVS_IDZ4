#include <iostream>
#include <array>
#include <csignal>
#include <getopt.h>
#include <cstring>
#include <fstream>

pthread_t tid[2];
pthread_mutex_t mutex;

struct Point {
    int x;
    int y;

    Point(int x, int y) {
        this->x = x;
        this->y = y;
    }

    explicit Point() {
        this->x = 0;
        this->y = 0;
    }

    bool operator==(Point right) const {
        return this->x == right.x && this->y == right.y;
    }
};

enum direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

enum cell { // типы квадратов
    NOT_GARDENED, // не ухоженный квадрат сада, но доступный для ухаживания
    GARDENED, // уже ухоженный квадрат сада
    ROCK, // камень
    POND, // пруд
    BEING_GARDENED // квадрат, за которым в данный момент ухаживает садовник
};

class Field {
public:
    void placeTheObstacles() { // создаем нужное количество препятствий на поле
        srand(time(nullptr));
        int amount = rand() % 21 + 10;
        for (int i = 0; i < amount; ++i) {
            int random_obstacle = rand() % 2;
            cell new_cell;
            if (random_obstacle == 0) {
                new_cell = ROCK;
            } else {
                new_cell = POND;
            }
            int random_x, random_y;
            do {
                random_x = rand() % 10;
                random_y = rand() % 10;
            } while (field_[random_x][random_y] != NOT_GARDENED);
            Point new_location(random_x, random_y);
            setCell(new_location, new_cell);
        }
    }

    void garden(int step_duration, Point currentSquare) { // садовник ухаживает за квадратом
        pthread_mutex_lock(&mutex);
        if (getCell(currentSquare) == NOT_GARDENED) {
            setCell(currentSquare, BEING_GARDENED);
            pthread_mutex_unlock(&mutex);
            usleep(step_duration * 2);
            setCell(currentSquare, GARDENED);
        } else {
            pthread_mutex_unlock(&mutex);
            usleep(step_duration);
        }
    }

    void printField(std::ofstream &output) { // выводим поле в файл
        pthread_mutex_lock(&mutex);
        for (int i = 9; i >= 0; --i) {
            for (int j = 0; j < 10; ++j) {
                Point current(j, i);
                pickColor(current, output);
            }
            output << std::endl;
        }
        pthread_mutex_unlock(&mutex);
    }

    void printField(Point first_gardener_location, Point second_gardener_location) { // выводим поле в консоль
        pthread_mutex_lock(&mutex);
        for (int i = 9; i >= 0; --i) {
            for (int j = 0; j < 10; ++j) {
                Point current(j, i);
                if (current == first_gardener_location && current == second_gardener_location) {
                    pickColor(current, "43");
                } else if (current == first_gardener_location) {
                    pickColor(current, "102");
                } else if (current == second_gardener_location) {
                    pickColor(current, "103");
                } else {
                    pickColor(current, "42");
                }
            }
            std::cout << std::endl;
        }
        pthread_mutex_unlock(&mutex);
    }

    cell getCell(Point point) {
        return field_[point.x][point.y];
    }

    void setCell(Point point, cell value) {
        field_[point.x][point.y] = value;
    }

private:
    std::array<std::array<cell, 10>, 10> field_ = {{{NOT_GARDENED}}}; // поле сада
    void pickColor(Point current,
                   std::ofstream &output) { // выводим клетку в файл с фоном нужного цвета, в зависимости от того есть ли на ней садовники

        switch (getCell(current)) {
            case NOT_GARDENED:
                output << "  ";
                break;
            case GARDENED:
                output << "\xF0\x9F\x8C\xBB";
                break;
            case ROCK:
                output << "\xF0\x9F\x97\xBF";
                break;
            case POND:
                output << "\xF0\x9F\x94\xB5";
                break;
            case BEING_GARDENED:
                output << "\xF0\x9F\x9A\xA7";
                break;
        }
    }

    void pickColor(Point current,
                   const std::string &color) { // выводим клетку в консоль с фоном нужного цвета, в зависимости от того есть ли на ней садовники
        switch (getCell(current)) {
            case NOT_GARDENED:
                std::cout << "\033[" + color + "m   \033[m";
                break;
            case GARDENED:
                std::cout << "\033[" + color + "m\xF0\x9F\x8C\xBB \033[m";
                break;
            case ROCK:
                std::cout << "\033[" + color + "m\xF0\x9F\x97\xBF \033[m";
                break;
            case POND:
                std::cout << "\033[" + color + "m\xF0\x9F\x94\xB5 \033[m";
                break;
            case BEING_GARDENED:
                std::cout << "\033[" + color + "m\xF0\x9F\x9A\xA7 \033[m";
                break;
        }
    }
};

class Gardener {
public:
    Point current_location; // текущее местоположение садовника
    bool haveFinished = false;

    void move() { // садовник делает ход
        (*field).garden(step_duration, current_location);
        if (current_location == ending_point) { // если обошли все клетки, то останавливаемся
            haveFinished = true;
            return;
        }
        if (!checkIfTheWayIsBlocked()) { // если подошли к краю, то идем в другое направление
            moveInDirection(current_direction);
        } else {
            moveInDirection(direction_when_blocked);
            if (current_direction == first_direction) { // меняем стороны в которые идут садовники на противоположные
                current_direction = second_direction;
            } else {
                current_direction = first_direction;
            }
        }
    }

    Gardener(Point starting_point, Point ending_point, direction first_direction,
             direction direction_when_blocked, direction second_direction,
             Field *field, int step_duration) {
        this->current_location = starting_point;
        this->ending_point = ending_point;
        this->first_direction = first_direction;
        this->second_direction = second_direction;
        this->current_direction = first_direction;
        this->direction_when_blocked = direction_when_blocked;
        this->field = field;
        this->step_duration = step_duration;
    }

private:
    Point ending_point; // точка, в которой садовник окажется, когда обойдет все клетки
    direction first_direction; // сначала идем в этом направлении
    direction second_direction; // когда дошли до края и развернулись, идем в этом направлении, потом меняем обратно
    direction current_direction; // текущее направление
    direction direction_when_blocked; // идем в этом направлении, когда подошли к краю
    Field *field; // указатель на поле сада
    int step_duration; // длительность того сколько занимает прохождение по необрабатываемой клетке, обрабатывание клетки x2 от step_duration

    bool checkIfTheWayIsBlocked() { // проверяем подошли ли к краю
        return current_direction == RIGHT && current_location.x == 9 ||
               current_direction == UP && current_location.y == 9 ||
               current_direction == LEFT && current_location.x == 0 ||
               current_direction == DOWN && current_location.y == 0;
    }

    void moveInDirection(direction direction) { // двигаемся в нужном направлении
        switch (direction) {
            case UP:
                while ((*field).getCell(Point(current_location.x, current_location.y + 1)) == BEING_GARDENED) {
                    printf(""); // ждем пока освободится, чтобы пройти
                }
                current_location.y += 1;
                break;
            case DOWN:
                while ((*field).getCell(Point(current_location.x, current_location.y - 1)) == BEING_GARDENED) {
                    printf(""); // ждем пока освободится, чтобы пройти
                }
                current_location.y -= 1;
                break;
            case LEFT:
                while ((*field).getCell(Point(current_location.x - 1, current_location.y)) == BEING_GARDENED) {
                    printf(""); // ждем пока освободится, чтобы пройти
                }
                current_location.x -= 1;
                break;
            case RIGHT:
                while ((*field).getCell(Point(current_location.x + 1, current_location.y)) == BEING_GARDENED) {
                    printf(""); // ждем пока освободится, чтобы пройти
                }
                current_location.x += 1;
                break;
        }
    }
};

static void *startGardening(void *arg) { // запускаем садовника
    while (!(*(Gardener *) arg).haveFinished) {
        (*(Gardener *) arg).move();
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    static struct option longOptions[] = {
            {"files",  required_argument, nullptr, 'f'},
            {"random", no_argument, nullptr, 'r'},
            {"std",    no_argument,       nullptr, 's'}
    };
    int optionIndex = 0;
    int arg = getopt_long(argc, argv, "f:rs", longOptions, &optionIndex);
    Field my_field; // создаем поле
    my_field.placeTheObstacles(); // расставляем препятствия
    switch (arg) {
        case 'f': {
            std::ifstream in_stream;
            std::ofstream out_stream;
            in_stream.open(strtok(optarg, ":"));
            out_stream.open(strtok(nullptr, ":"));
            double speed_first, speed_second;
            in_stream >> speed_first;
            in_stream >> speed_second;
            int step_duration_first = 1000000 / speed_first;
            int step_duration_second = 1000000 / speed_second;
            Gardener first_gardener(Point(0, 9), Point(0, 0),
                                    RIGHT, DOWN, LEFT,
                                    &my_field, step_duration_first); // создаем садовников
            Gardener second_gardener(Point(9, 0), Point(0, 0),
                                     UP, LEFT, DOWN,
                                     &my_field, step_duration_second);
            pthread_create(&(tid[0]), nullptr, startGardening,
                           &first_gardener); // запускаем садовников в разных потоках
            pthread_create(&(tid[1]), nullptr, startGardening, &second_gardener);
            while (!first_gardener.haveFinished || !second_gardener.haveFinished) {
                my_field.printField(out_stream);
                my_field.printField(first_gardener.current_location, second_gardener.current_location);
                std::cout << std::endl;
                out_stream << std::endl;
                usleep(std::min(step_duration_first, step_duration_second));
            }
            pthread_join(tid[0], nullptr);
            pthread_join(tid[1], nullptr);
            in_stream.close();
            out_stream.close();
            break;
        }
        case 'r': {
            srand(time(nullptr));
            int speed_first = rand() % 100 + 1;
            int speed_second = rand() % 100 + 1;
            std::cout << "Following numbers were generated: " + std::to_string(speed_first) + " " + std::to_string(speed_second) << std::endl;
            int step_duration_first = 1000000 / speed_first;
            int step_duration_second = 1000000 / speed_second;
            Gardener first_gardener(Point(0, 9), Point(0, 0),
                                    RIGHT, DOWN, LEFT,
                                    &my_field, step_duration_first); // создаем садовников
            Gardener second_gardener(Point(9, 0), Point(0, 0),
                                     UP, LEFT, DOWN,
                                     &my_field, step_duration_second);
            pthread_create(&(tid[0]), nullptr, startGardening,
                           &first_gardener); // запускаем садовников в разных потоках
            pthread_create(&(tid[1]), nullptr, startGardening, &second_gardener);
            while (!first_gardener.haveFinished || !second_gardener.haveFinished) {
                my_field.printField(first_gardener.current_location, second_gardener.current_location);
                std::cout << std::endl;
                usleep(std::min(step_duration_first, step_duration_second));
            }
            pthread_join(tid[0], nullptr);
            pthread_join(tid[1], nullptr);
            break;
        }
        case 's': {
            std::cout
                    << "Введите скорости (квадратов в секунду), с которыми будут работать садовники (2 положительных рациональных числа, разделенным пробелом):"
                    << std::endl;
            double speed_first, speed_second;
            std::cin >> speed_first;
            std::cin >> speed_second;
            int step_duration_first = 1000000 / speed_first;
            int step_duration_second = 1000000 / speed_second;
            Gardener first_gardener(Point(0, 9), Point(0, 0),
                                    RIGHT, DOWN, LEFT,
                                    &my_field, step_duration_first); // создаем садовников
            Gardener second_gardener(Point(9, 0), Point(0, 0),
                                     UP, LEFT, DOWN,
                                     &my_field, step_duration_second);
            pthread_create(&(tid[0]), nullptr, startGardening,
                           &first_gardener); // запускаем садовников в разных потоках
            pthread_create(&(tid[1]), nullptr, startGardening, &second_gardener);
            while (!first_gardener.haveFinished || !second_gardener.haveFinished) {
                my_field.printField(first_gardener.current_location, second_gardener.current_location);
                std::cout << std::endl;
                usleep(std::min(step_duration_first, step_duration_second));
            }
            pthread_join(tid[0], nullptr);
            pthread_join(tid[1], nullptr);
            break;
        }
        default: {
            int step_duration_first = 1000000 / std::stod(argv[1]);
            int step_duration_second = 1000000 / std::stod(argv[2]);
            Gardener first_gardener(Point(0, 9), Point(0, 0),
                                    RIGHT, DOWN, LEFT,
                                    &my_field, step_duration_first); // создаем садовников
            Gardener second_gardener(Point(9, 0), Point(0, 0),
                                     UP, LEFT, DOWN,
                                     &my_field, step_duration_second);
            pthread_create(&(tid[0]), nullptr, startGardening,
                           &first_gardener); // запускаем садовников в разных потоках
            pthread_create(&(tid[1]), nullptr, startGardening, &second_gardener);
            while (!first_gardener.haveFinished || !second_gardener.haveFinished) {
                my_field.printField(first_gardener.current_location, second_gardener.current_location);
                std::cout << std::endl;
                usleep(std::min(step_duration_first, step_duration_second));
            }
            pthread_join(tid[0], nullptr);
            pthread_join(tid[1], nullptr);
        }
    }
    return 0;
}
