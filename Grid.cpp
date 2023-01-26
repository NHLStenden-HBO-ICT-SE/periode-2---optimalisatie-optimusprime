//
// Created by Sven Imholz on 09/01/2023.
//
#include "precomp.h"
#include "Grid.h"

void Grid::add(Tank *tank) {
    vec2 cellPos(tank->position.x / CELL_SIZE, tank->position.y / CELL_SIZE);
    grid.at(cellPos.x).at(cellPos.y).push_back(tank);
}

void Grid::remove(Tank *tank) {
    vec2 cellPos(tank->position.x / CELL_SIZE, tank->position.y / CELL_SIZE);

    for (int i = 0; i < grid.at(cellPos.x).at(cellPos.y).size(); ++i) {
        if (grid.at(cellPos.x).at(cellPos.y).at(i) == tank) {
            grid.at(cellPos.x).at(cellPos.y).erase(grid.at(cellPos.x).at(cellPos.y).begin() + i);
            break;
        }
    }
}

void Grid::update(Tank* tank) {
    remove(tank);
    add(tank);
}

std::vector<Tank *> Grid::getTanksAt(vec2 cellPosition) {
    return grid.at(cellPosition.x).at(cellPosition.y);
}