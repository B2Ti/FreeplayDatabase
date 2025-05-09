import json

all_bloons = {
    "Red":    0b00001_000,
    "Blue":   0b00010_000,
    "Green":  0b00011_000,
    "Yellow": 0b00100_000,
    "Pink":   0b00101_000,
    "White":  0b00110_000,
    "Black":  0b00111_000,
    "Purple": 0b01000_000,
    "Zebra":  0b01001_000,
    "Lead":   0b01010_000,
    "Rainbow":0b01011_000,
    "Ceramic":0b01100_000,
    "Moab":   0b01101_000,
    "Bfb":    0b01110_000,
    "Zomg":   0b01111_000,
    "Ddt":    0b10000_000,
    "Bad":    0b10001_000
}

modifiers = {
    "Fortified" : 0b100,
    "Camo"      : 0b010,
    "Regrow"    : 0b001
}

def bloon_StoI(bloon:str) -> int:
    value = 0
    for modStr in modifiers.keys():
        if modStr in bloon:
            value += modifiers[modStr]
            bloon = bloon.replace(modStr, "")
    value += all_bloons[bloon]
    return value

with open("cleanedFreeplayGroups2.json", 'r') as f:
    allGroups = json.load(f)

#arrayGroups:list[tuple[float, int, int, list[int]]] = []

class Score:
    def __repr__(self):
        return ".score"
class Type_:
    def __repr__(self):
        return ".type"
class Count:
    def __repr__(self):
        return ".count"
class Bounds:
    def __repr__(self):
        return ".bounds"

arrayGroups:list[dict[Score|Type_|Count|Bounds, float|int|int|list[int]]] = []
boundlessGroups:list[dict[Score|Type_|Count, float|int|int|list[int]]] = []

for group in allGroups:
    score = group["score"]
    bloons = group["group"]
    bloon = bloon_StoI(bloons["bloon"])
    count = bloons["count"]
    bounds_json:list[dict[str, int]] = group["bounds"]
    bounds_length = 10
    bounds_array = [value for bound in bounds_json for value in reversed(bound.values())]
    bounds_array += [-1] * (bounds_length - len(bounds_array))
    arrayGroups.append({Score():float(score), Type_():bloon, Count():count, Bounds():bounds_array})
    boundlessGroups.append({Score():float(score),Type_():bloon, Count():count})

prevRoundMask = 0
prevCount = 0
for round_ in range(141, 1000):
    count = 0
    roundMask = 0
    for groupIdx in range(len(arrayGroups)):
        group = arrayGroups[groupIdx]
        inBounds = False
        bounds = list(group.values())[-1]
        for lbound, ubound in zip(bounds[::2], bounds[1::2]):
            if lbound <= round_ and round_ <= ubound:
                inBounds = True
                break
        if inBounds:
            count += 1
            roundMask |= 1 << groupIdx
    if roundMask != prevRoundMask:
        print(f"round {round_} has a new bound set")
    if prevCount != count:
        print(f"round {round_} has a new count: {count}")
        prevCount = count
    prevRoundMask = roundMask