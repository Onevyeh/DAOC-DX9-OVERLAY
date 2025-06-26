#include <set>
#include <cstdint>
#include <niflib.h>
#include <obj/NiTriShape.h>
#include "injectemesh.h"
#undef byte
#include <cstdint>

// hash herbes sprites
std::set<uint32_t> knownVegetationHashes = {
    0x101ECAB5, // Exemple hash, remplace par les tiens
    0xED42CB33, //grassalb.dds d'origine
    0x5D685660,   //grasshib.dds d'origine
    0x7866570,       //grassabl test sans alpha
   0x243C367C     // sol pour vertex


       
};

//hash arbres
std::set<uint32_t> knownLeafTextures = { 

    0x2EB26940,       //feuilles arbre alb
    0xFBD49D70,      //feuilles arbre alb
    0x55E7CEAE






};



void LoadKnownLeafTextures() {
    // Cette fonction peut être vide si les textures sont statiques
}
