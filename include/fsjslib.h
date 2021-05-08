// Evite que fsjslib.h soit inclus plusieurs fois (à chaque #include "fsjslib.h")
#ifndef FSJSLIB_H
#define FSJSLIB_H


#ifdef __cplusplus // Bibliothèque compilable et utilisable en C++
extern "C" {
#endif


#ifdef BUILDING_DLL // "commutateur" : compilation DLL / compilation application cliente
#define fsjslib __declspec(dllexport) // "fsjslib" remplacé par "__declspec(dllexport)" lors de la compilation de la DLL
#else
#define fsjslib __declspec(dllimport) // "fsjslib" remplacé par "__declspec(dllimport)" lors de la compilation de l'application cliente
#endif


/**********************************************
 **************** Définitions *****************
 **********************************************
*/
// Modèles d'énumérations
typedef enum eType eType;
enum eType
{
	typeInteger, typeFloat
};

// Modèles de structures
typedef struct sfEisIntFloat sfEisIntFloat;
struct sfEisIntFloat
{
	const char* pcharEntry;
	eType type;
	double min;
	double max;
	int errNumber;
	char* pcharErrMessage;
	int intResult;
	double doubleResult;
};

// Modèles d'unions


/**********************************************
 ********* Prototypes des fonctions ***********
 ********** importées / exportées *************
 **********************************************
*/
void fsjslib fEntryIsIntOrFloat(sfEisIntFloat* psfEisIntFloat);


/**********************************************
 ****** Prototypes des fonctions internes******
 **********************************************
*/
char fIsIntOrFloat(const char* pSaisie,eType type);


#ifdef __cplusplus
}
#endif

#endif  // FSJSLIB_H
