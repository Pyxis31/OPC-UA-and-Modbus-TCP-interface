// Evite que callback.h soit inclus plusieurs fois (à chaque #include "callback.h")
#ifndef CALLBACK_H
#define CALLBACK_H


/**********************************************
 **************** Définitions *****************
 **********************************************
*/
// Constantes de préprocesseur, macros, etc.
#define MODBUS_PORT "502" // Port Modbus TCP

// Modèles de structures
// Structure de pointeurs pour les paramètres de certaines fonctions
typedef struct sParamFnc_np sParamFnc_np; // "sParamFnc_np" alias de "struct sParamFnc_np"
struct sParamFnc_np
{
	gpointer pData1;
	gpointer pData2;
	gpointer pData3;
};

// Modèles d'unions


// Modèles d'énumérations
typedef enum eConnection eConnection;
enum eConnection
{
	Disconnected, Connected
};


/**********************************************
 ********* Prototypes de fonctions ************
 **********************************************
*/
// Fonctions callback
G_MODULE_EXPORT void on_window1_map_event(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_window1_destroy_event(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_about_button_clicked(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_Connect_switch_state_set(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_read_button_clicked(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_write_button_clicked(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_PeriodicRead_CheckButton_toggled(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_Browse_button_clicked(GtkWidget* pWidget, gpointer pData);
G_MODULE_EXPORT void on_ListEndpoints_button_clicked(GtkWidget* pWidget, gpointer pData);

// Fonctions applicatives
void fMessage(char* pcharMessage);
gboolean fPeriodicRead(gpointer pData);
void fRead();

#endif
