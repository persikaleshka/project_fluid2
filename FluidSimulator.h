#pragma once

#include <array>
#include <iostream>
#include <random>
#include <cassert>       
#include <algorithm>     
#include <tuple>         
#include <utility>  
#include <cstring>
#include <ostream>
#include <fstream> 

#include "fixed.h"

constexpr std::array<std::pair<int, int>, 4> deltas{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};

template<typename T, size_t N = 32, size_t M = 84>
struct VectorField {
    std::array<T, deltas.size()> v[N][M];

    T& add(int x, int y, int dx, int dy, T dv) {
        return get(x, y, dx, dy) += dv;
    }

    T& get(int x, int y, int dx, int dy) {
        auto it = std::find(deltas.begin(), deltas.end(), std::make_pair(dx, dy));
        assert(it != deltas.end());
        size_t i = std::distance(deltas.begin(), it);
        return v[x][y][i];
    }
};

template<typename Ptype, typename VType, typename VFlowType, size_t N = 36, size_t M = 84>
class Simulator {
public:
    Simulator() = default;
    void run_simulation(size_t T, size_t save_count, const std::string& file_name);

private:
    VType rho_[256] {};
    VType g_;
    int dirs[N][M]{};
    Ptype p[N][M]{}, old_p[N][M]{};

    std::vector<std::string> field;

    VectorField<VType, N, M> velocity;
    VectorField<VFlowType, N, M> velocity_flow;
    int last_use[N][M] {};
    int UT = 0;
    std::mt19937 random_generator_;

    struct ParticleParams {
        char type;
        Ptype cur_p;
        std::array<VType, deltas.size()> v;

        void swap_with(Simulator& fs, int x, int y) {
            std::swap(fs.field[x][y], type);
            std::swap(fs.p[x][y], cur_p);
            std::swap(fs.velocity.v[x][y], v);
        }
    };

    void readInputFile(const std::string& filename);

    std::tuple<VType, bool, std::pair<int, int>> propagate_flow(int x, int y, VType lim);
    double random01();
    void propagate_stop(int x, int y, bool force = false);
    Ptype move_prob(int x, int y); 
    bool propagate_move(int x, int y, bool is_first);
    void saveToJson(const std::string& filename) const;

};

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
void Simulator<Ptype, VType, VFlowType, N, M>::readInputFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть файл " << filename << std::endl;
        return;
    }

    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.find("\"g\"") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                g_ = std::stod(trim(line.substr(pos + 1)));
            }
        }
        else if (line.find("\"rho\"") != std::string::npos) {
            while (std::getline(file, line)) {
                line = trim(line);
                if (line.find("}") != std::string::npos) break;

                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string key = trim(line.substr(0, colonPos));
                    std::string value = trim(line.substr(colonPos + 1));

                    if (key.front() == '"' && key.back() == '"') {
                        key = key.substr(1, key.size() - 2);
                    }

                    char keyChar = key.empty() ? ' ' : key[0];
                    rho_[keyChar] = std::stod(value);
                }
            }
        }
        else if (line.find("\"field\"") != std::string::npos) {
            while (std::getline(file, line)) {
                line = trim(line);
                if (line.find("]") != std::string::npos) break; 

                if (!line.empty()) {
                    if (line.back() == ',') {
                        line = line.substr(0, line.size() - 1);
                    }

                    if (line.front() == '"' && line.back() == '"') {
                        line = line.substr(1, line.size() - 2);
                    }

                    field.push_back(line);
                }
            }
        }
    }

    file.close();
    
    std::cout << "Поле загружено. Размер: " << field.size() << " строк." << std::endl;
}


template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
std::tuple<VType, bool, std::pair<int, int>> 
Simulator<Ptype, VType, VFlowType, N, M>::propagate_flow(int x, int y, VType lim)  
{
    this->last_use[x][y] = this->UT - 1;
    VType ret = 0;
    for (auto [dx, dy] : deltas) {
        int nx = x + dx, ny = y + dy;
        if (this->field[nx][ny] != '#' && this->last_use[nx][ny] < this->UT) {
            auto cap = this->velocity.get(x, y, dx, dy);
            auto flow = this->velocity_flow.get(x, y, dx, dy);
            if (flow == cap) {
                continue;
            }
            auto vp = std::min(static_cast<VFlowType>(lim), static_cast<VFlowType>(cap) - flow);

            if (this->last_use[nx][ny] == this->UT - 1) {
                this->velocity_flow.add(x, y, dx, dy, vp);
                this->last_use[x][y] = this->UT;
                return {static_cast<VType>(vp), 1, {nx, ny}};
            }
            auto [t, prop, end] = this->propagate_flow(nx, ny, static_cast<VType>(vp));
            ret += t;
            if (prop) {
                this->velocity_flow.add(x, y, dx, dy, static_cast<VFlowType>(t));
                this->last_use[x][y] = this->UT;
                return {t, prop && end != std::make_pair(x, y), end};
            }
        }
    }
    this->last_use[x][y] = this->UT;
    return {ret, 0, {0, 0}};
};

template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
double Simulator<Ptype, VType, VFlowType, N, M>::random01()
{       
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(random_generator_);
}

template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
void Simulator<Ptype, VType, VFlowType, N, M>::propagate_stop(int x, int y, bool force)
{
    if (!force) {
        bool stop = true;
        for (auto [dx, dy] : deltas) {
            int nx = x + dx, ny = y + dy;
            if (field[nx][ny] != '#' && last_use[nx][ny] < UT - 1 && velocity.get(x, y, dx, dy) > 0) {
                stop = false;
                break;
            }
        }
        if (!stop) {
            return;
        }
    }
    last_use[x][y] = UT;
    for (auto [dx, dy] : deltas) {
        int nx = x + dx, ny = y + dy;
        if (field[nx][ny] == '#' || last_use[nx][ny] == UT || velocity.get(x, y, dx, dy) > 0) {
            continue;
        }
        propagate_stop(nx, ny);
    }
}

template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
Ptype Simulator<Ptype, VType, VFlowType, N, M>::move_prob(int x, int y)
{
    Ptype sum = 0;
    for (size_t i = 0; i < deltas.size(); ++i) {
        auto [dx, dy] = deltas[i];
        int nx = x + dx, ny = y + dy;
        if (field[nx][ny] == '#' || last_use[nx][ny] == UT) {
            continue;
        }
        auto v = velocity.get(x, y, dx, dy);
        if (v < 0) {
            continue;
        }
        sum += v;
    }
    return sum;
}

template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
bool Simulator<Ptype, VType, VFlowType, N, M>::propagate_move(int x, int y, bool is_first)
{
    last_use[x][y] = UT - is_first;
    bool ret = false;
    int nx = -1, ny = -1;
    do {
        std::array<Ptype, deltas.size()> tres;
        Ptype sum = 0;
        for (size_t i = 0; i < deltas.size(); ++i) {
            auto [dx, dy] = deltas[i];
            int nx = x + dx, ny = y + dy;
            if (field[nx][ny] == '#' || last_use[nx][ny] == UT) {
                tres[i] = sum;
                continue;
            }
            auto v = velocity.get(x, y, dx, dy);
            if (v < 0) {
                tres[i] = sum;
                continue;
            }
            sum += v;
            tres[i] = sum;
        }

        if (sum == 0) {
            break;
        }

        Ptype p = sum * random01();
        size_t d = std::ranges::upper_bound(tres, p) - tres.begin();

        auto [dx, dy] = deltas[d];
        nx = x + dx;
        ny = y + dy;
        assert(velocity.get(x, y, dx, dy) > 0 && field[nx][ny] != '#' && last_use[nx][ny] < UT);

        ret = (last_use[nx][ny] == UT - 1 || propagate_move(nx, ny, false));
    } while (!ret);
    last_use[x][y] = UT;
    for (size_t i = 0; i < deltas.size(); ++i) {
        auto [dx, dy] = deltas[i];
        int nx = x + dx, ny = y + dy;
        if (field[nx][ny] != '#' && last_use[nx][ny] < UT - 1 && velocity.get(x, y, dx, dy) < 0) {
            propagate_stop(nx, ny);
        }
    }
    if (ret) {
        if (!is_first) {
            ParticleParams pp{};
            pp.swap_with(*this, x, y);
            pp.swap_with(*this, nx, ny);
            pp.swap_with(*this, x, y);
        }
    }
    return ret;
}

template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
void Simulator<Ptype, VType, VFlowType, N, M>::saveToJson(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл для записи: " << filename << std::endl;
        return;
    }

    file << "{\n";  
    
    file << "  \"g\": " << g_ << ",\n";

    file << "  \"rho\": {\n";
    file << "    \" \": " << rho_[' '] << ",\n";
    file << "    \".\": " << rho_['.'] << "\n";
    file << "  },\n";

    file << "  \"field\": [\n";
    for (size_t i = 0; i < field.size(); ++i) {
        file << "    \"" << field[i] << "\"";
        if (i != field.size() - 1)
            file << ",";
        file << "\n";
    }
    file << "  ]\n";

    file << "}\n";

    file.close();

}


template<typename Ptype, typename VType, typename VFlowType, size_t N, size_t M>
void Simulator<Ptype, VType, VFlowType, N, M>::run_simulation(size_t T, size_t save_interval, const std::string& file_name)
{
    if (file_name.size() == 0) {
        readInputFile("../input.json");
    } else {
        readInputFile(file_name);
    }

    if (rho_[' '] == 0 || g_ == 0) {
        std::cout << "Слишком маленькая точность, переменные равны 0\n";
        return;
    } else if (rho_['.'] <= 0) {
        std::cout << "Слишком маленькая точность, переменные переполнились\n";
        return;
    }
    

    for (size_t x = 0; x < field.size(); ++x) {
            for (size_t y = 0; y < field[0].size(); ++y) {
                if (field[x][y] == '#')
                    continue;
                for (auto [dx, dy] : deltas) {
                    dirs[x][y] += (field[x + dx][y + dy] != '#');
                }
            }
        }

    for (size_t i = 0; i < T; ++i) {

        Ptype total_delta_p = 0;    
        for (size_t x = 0; x < field.size(); ++x) {
            for (size_t y = 0; y < field[0].size(); ++y) {
                if (field[x][y] == '#')
                    continue;
                if (field[x + 1][y] != '#')
                    velocity.add(x, y, 1, 0, g_);
            }
        }

        memcpy(this->old_p, this->p, sizeof(this->p));
        for (size_t x = 0; x < field.size(); ++x) {
            for (size_t y = 0; y < field[0].size(); ++y) {
                if (field[x][y] == '#')
                    continue;
                for (auto [dx, dy] : deltas) {
                    int nx = x + dx, ny = y + dy;
                    if (field[nx][ny] != '#' && old_p[nx][ny] < old_p[x][y]) {
                        auto delta_p = old_p[x][y] - old_p[nx][ny];
                        auto force = delta_p;
                        auto &contr = velocity.get(nx, ny, -dx, -dy);
                        if (force <= contr * rho_[(int) field[nx][ny]]) {
                            contr -= static_cast<VType>(static_cast<VType>(force) / rho_[(int) field[nx][ny]]);
                            continue;
                        }
                        force -= contr * rho_[(int) field[nx][ny]];
                        contr = 0;
                        velocity.add(x, y, dx, dy, static_cast<VType>(force) / rho_[(int) field[x][y]]);
                        p[x][y] -= force / dirs[x][y];
                        total_delta_p -= force / dirs[x][y];
                    }
                }
            }
        }

        velocity_flow = {};
        bool prop = false;
        do {
            UT += 2;
            prop = 0;
            for (size_t x = 0; x < field.size(); ++x) {
                for (size_t y = 0; y < field[0].size(); ++y) {
                    if (field[x][y] != '#' && last_use[x][y] != UT) {
                        auto [t, local_prop, _] = propagate_flow(x, y, 1);
                        if (t > 0) {
                            prop = 1;
                        }
                    }
                }
            }
        } while (prop);

        for (size_t x = 0; x < field.size(); ++x) {
            for (size_t y = 0; y < field[0].size(); ++y) {
                if (field[x][y] == '#')
                    continue;
                for (auto [dx, dy] : deltas) {
                    auto old_v = velocity.get(x, y, dx, dy);
                    auto new_v = velocity_flow.get(x, y, dx, dy);
                    if (old_v > 0) {
                        assert(static_cast<float>(new_v) <= static_cast<float>(old_v));
                        velocity.get(x, y, dx, dy) = static_cast<VType>(new_v);
                        auto force = (static_cast<VFlowType>(old_v) - new_v) * rho_[(int) field[x][y]];
                        if (field[x][y] == '.')
                            force *= 0.8;
                        if (field[x + dx][y + dy] == '#') {
                            p[x][y] += force / dirs[x][y];
                            total_delta_p += force / dirs[x][y];
                        } else {
                            p[x + dx][y + dy] += force / dirs[x + dx][y + dy];
                            total_delta_p += force / dirs[x + dx][y + dy];
                        }
                    }
                }
            }
        }

        UT += 2;
        prop = false;
        for (size_t x = 0; x < field.size(); ++x) {
            for (size_t y = 0; y < field[0].size(); ++y) {
                if (field[x][y] != '#' && last_use[x][y] != UT) {
                    if (move_prob(x, y) > random01()) {
                        prop = true;
                        propagate_move(x, y, true);
                    } else {
                        propagate_stop(x, y, true);
                    }
                }
            }
        }
        if ((i + 1) % save_interval == 0) {
            std::string filename = "../output.json";
            saveToJson(filename);
        }

        if (prop) {
            std::cout << "tick " << i << ":\n";
            for (size_t x = 0; x < field.size(); ++x) {
                std::cout << field[x] << "\n";
            }
        }
    }
    std::cout << "end" << std::endl;
}