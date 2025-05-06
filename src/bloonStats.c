#include <bloonStats.h>

static const double bloonCash[18][2] = {
    {0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}, {3.0, 3.0}, {4.0, 4.0}, {5.0, 5.0},
    {11.0, 6.0}, {11.0, 6.0}, {11.0, 6.0}, {23.0, 7.0}, {23.0, 7.0},
    {47.0, 8.0}, {95.0, 95.0}, {381.0, 381.0}, {1525.0, 1525.0},
    {6101.0, 6101.0}, {381.0, 381.0}, {13346.0,13346.0}
};

static const char *bloonNames[18] = {
    "", 
    "Red",
    "Blue",
    "Green",
    "Yellow",
    "Pink",
    "Black",
    "White",
    "Purple",
    "Zebra",
    "Lead",
    "Rainbow",
    "Ceramic",
    "Moab",
    "Bfb",
    "Zomg",
    "Ddt",
    "Bad"
};

static const char *bloonModifiers[8] = {
    "",
    "Regrow",
    "Camo",
    "Camgrow",
    "Fortified",
    "FortifiedRegrow",
    "FortifiedCamo",
    "FortifiedCamgrow"
};

const char *getBloonModifiers(int32_t bloontype){
    return bloonModifiers[bloontype & 0x7];
}

const char *getBloonName(int32_t bloontype){
    return bloonNames[bloontype >> 3];
}

double cashMultiplier(int32_t round){
    if (round > 120) return 0.02;
    if (round <= 50) return 1.0;
    if (round <= 60) return 0.5;
    if (round <= 85) return 0.2;
    if (round <= 100) return 0.1;
    if (round <= 120) return 0.05;
    return 0.02;
}

double getCash(int32_t bloontype, int32_t round){
    int freeplay = round > 80;
    int bloontypeI = (int)(bloontype) >> 3;
    double cash = bloonCash[bloontypeI][freeplay];
    double mult = cashMultiplier(round);
    return cash * mult;
}
