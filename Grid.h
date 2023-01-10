//
// Created by Sven Imholz on 09/01/2023.
//

#ifndef TMPL8_2018_01_GRID_H
#define TMPL8_2018_01_GRID_H


class Grid {
public:
    Grid() {

    }

    Grid(int num_cells, int cell_size) {
        NUM_CELLS = num_cells;
        CELL_SIZE = cell_size;

        for (int x = 0; x < NUM_CELLS; ++x) {
            std::vector<std::vector<Tank *>> col;
            for (int y = 0; y < NUM_CELLS; ++y) {
                col.push_back(std::vector<Tank *>());
            }
            grid.push_back(col);
        }
    }

    void add(Tank *tank);

    void update(Tank *tank);

    void remove(Tank *tank);

    std::vector<Tank *> getTanksAt(vec2 cellPosition);


    int NUM_CELLS;
    int CELL_SIZE;

private:
    std::vector<std::vector<std::vector<Tank *>>> grid;
};


#endif //TMPL8_2018_01_GRID_H
