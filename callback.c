/**********************************************
 ********** Directives d'inclusions ***********
 ************ (du préprocesseur) **************
 **********************************************
*/

// Des Headers situés dans les répertoires listés dans le makefile
#include <gtk/gtk.h>	// Gimp Tool Kit
#include <stdio.h>		// sprintf, FILE, fopen, fclose
#include <stdlib.h>		// atoi, malloc, calloc, sizeof, free
#include <string.h>		// strlen, strcmp
// #include <ctype.h>		// isdigit
#include <winsock2.h>	// Sockets TCP (avec la librairie ws2_32.lib)
#include <windows.h> 	// ShellExecute
#include <shellapi.h> 	// ShellExecute
#include <float.h>		// Limites
// #include <mmsystem.h>	// Sons
#include <modbus.h>		// Modbus RTU et TCP
#include <open62541.h>	// OPC UA
#include <fsjslib.h>	// Ma librairie

// Des Headers situés dans le répertoire de l'application
#include "callback.h"


/**********************************************
 ********** Déclarations (globales) ***********
 **********************************************
*/

// Externes   
extern GtkBuilder* pConstInterface; // pConstInterface vient d'un autre fichier source

// Types de base

// Autres types 
eConnection Connection=Disconnected;

// Pointeurs sur chaînes
const char* pcharIPaddress;

// Pointeurs de structures
FILE* pErrOut;
modbus_t* pPlc_MBclient;
UA_Client* pPlc_OPCclient;

// Pointeurs de widgets de l'interface
GtkWidget 	*pMaFenetre, *pIP_address_entry, *pRegister_address_entry, *pNumber_of_Registers_entry,
			*pAbout_dialog, *pMessages_display, *pConnect_switch,
			*pWrite_button, *pRead_button, *pRead_value, *pWrite_value,
			*pRegValues_TextView, *pPeriodicRead_CheckButton, *pOPC_radiobutton,
			*pBrowse_button, *pBrowseObj_TextView, *pNamespace_entry, *pName_entry, *pType_comboBoxText;

	
/**********************************************
 ************ Fonctions callback **************
 **********************************************
*
* Ces fonctions sont liées à des événements (button click, etc.)
* Remarque : la macro G_MODULE_EXPORT exporte la fonction associée dans une table de symboles d'application au moment de la compilation
* Cette table est en suite surveillée par le "gtk_builder_connect_signals" du main.c
*
*/


// Ouverture de la fenêtre principale
G_MODULE_EXPORT void on_window1_map_event(GtkWidget* pWidget, gpointer pData)
{
	// ********** Déclarations (locales) ************

	// *********** Fin de déclarations **************
		
	// Initialise les variables

	
	// Initialise les pointeurs des widgets de l'interface
	pMaFenetre=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"window1"));
	pIP_address_entry=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"IP_address_entry"));
	pRegister_address_entry=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Register_address_entry"));
	pNumber_of_Registers_entry=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Number_of_Registers_entry"));
	pAbout_dialog=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"About_dialog"));
	pMessages_display=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Messages_display"));
	pConnect_switch=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Connect_switch"));
	pRegValues_TextView=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"RegValues_TextView"));
	pWrite_value=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Write_value"));
	pPeriodicRead_CheckButton=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"PeriodicRead_CheckButton"));
	pOPC_radiobutton=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"OPC_radiobutton"));
	pBrowseObj_TextView=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"BrowseObj_TextView"));
	pNamespace_entry=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Namespace_entry"));
	pName_entry=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Name_entry"));
	pType_comboBoxText=GTK_WIDGET(gtk_builder_get_object(pConstInterface,"Type_comboBoxText"));
	
	// Ouvre le fichier errLog
	pErrOut=fopen("CommInterface_errLog.txt", "w");

	// Appel périodique de fonction
	g_timeout_add(1000,fPeriodicRead,pData);
}


// Fermeture de la fenêtre principale
G_MODULE_EXPORT void on_window1_destroy_event(GtkWidget* pWidget, gpointer pData)
{
	// Ferme le fichier errLog
	fclose(pErrOut);

	// Clôture la connexion et libère les ressources
	Connection=Disconnected;
	modbus_close(pPlc_MBclient);	
	modbus_free(pPlc_MBclient);
	UA_Client_delete(pPlc_OPCclient);
}


// Menu "A propos"
G_MODULE_EXPORT void on_about_button_clicked(GtkWidget* pWidget, gpointer pData)
{
	// ********** Déclarations (locales) ************

	// *********** Fin de déclarations **************
	
	// Affiche (show) le widget et bloc l'exécution du programme dans l'attente d'une réponse
	gtk_dialog_run(GTK_DIALOG(pAbout_dialog));
	
	/*
	 * Dès que la fonction gtk_dialog_run sort de sa boucle récursive,
	 * cette fonction cache le widget mais ne le détruit pas
	 */
	gtk_widget_hide(pAbout_dialog);
}


// Commutateur de connexion/déconnexion au serveur
G_MODULE_EXPORT void on_Connect_switch_state_set(GtkWidget* pWidget, gpointer pData)
{
	// ********** Déclarations (locales) ************
	char pcharMessage[50+1]="";
	gboolean ConnectionSwitch;
	gboolean MB_OPC_RadioButton;
	char pcharStrUAconnect[30+1];
	// *********** Fin de déclarations **************

	// Récupère l'état du switch de connexion et du groupe de boutons radio Modbus/OPC
	ConnectionSwitch=gtk_switch_get_active(GTK_SWITCH(pConnect_switch));
	MB_OPC_RadioButton=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pOPC_radiobutton));
	
	// Récupère le contenu alphanumérique des zones de saisies
	pcharIPaddress=gtk_entry_get_text(GTK_ENTRY(pIP_address_entry));

	if (ConnectionSwitch)
	/* Switch à ON */
	{
		if (inet_addr(pcharIPaddress)==-1)
		// L'adresse IP est incorrecte
		{
			sprintf(pcharMessage,"The IP address is invalid");
		}
		else
		{
			if (!MB_OPC_RadioButton)
			// Modbus TCP
			{
				// Initialise le pointeur de structure modbus_t avec l'adresse IP du serveur et le port Modbus
				pPlc_MBclient = modbus_new_tcp_pi(pcharIPaddress,MODBUS_PORT);
				// L'initialisation a échoué
				if (pPlc_MBclient == NULL)
				{
					sprintf(pcharMessage,"Unable to allocate libmodbus context"); // Formateur de texte printf vers une sortie string
					modbus_close(pPlc_MBclient);
				}
				// Le serveur ne répond pas
				if (modbus_connect(pPlc_MBclient) == -1)
				{
					sprintf(pcharMessage, "Modbus TCP server not responding");
					modbus_close(pPlc_MBclient);
				}
				// Connecté !
				else if(modbus_connect(pPlc_MBclient) == 0)
				{
					Connection=Connected;
					sprintf(pcharMessage,"Connection to the Modbus TCP server successful !");
				}
			}
			else
			// OPC UA
			{
				// Initialise le pointeur de structure UA_Client
				pPlc_OPCclient=UA_Client_new(UA_ClientConfig_default);
				// Connexion avec l'adresse IP du serveur et le port OPC UA
				sprintf(pcharStrUAconnect,"opc.tcp://%s%s",pcharIPaddress,":4840");
				UA_StatusCode UAretval=UA_Client_connect(pPlc_OPCclient,pcharStrUAconnect);
				// Ou : UA_Client_connect_username(pcharStrUAconnect, "opc.tcp://localhost:4840", "user1", "password");
				// Le serveur ne répond pas
				if(UAretval!=UA_STATUSCODE_GOOD)
				{
					sprintf(pcharMessage,"OPC UA server not responding status = %i", UAretval);
					//UA_Client_delete(pPlc_OPCclient);
				}
				else
				// Connecté !
				{
					Connection=Connected;
					sprintf(pcharMessage,"Connection to the OPC UA server successful !");
				}	
			}
		}
	}
	else
	/* Switch à OFF */
	// Libère les ressources
	{
		if (Connection==Connected)
		{
			sprintf(pcharMessage,"Disconnected");
			Connection=Disconnected;
			modbus_close(pPlc_MBclient);
			UA_Client_delete(pPlc_OPCclient);
		}
	}
	// Affiche le message
	fMessage(pcharMessage);
}


// Bouton de lecture
G_MODULE_EXPORT void on_read_button_clicked(GtkWidget* pWidget, gpointer pData)
{
	fRead();
}


// Bouton listing endpoints
G_MODULE_EXPORT void on_ListEndpoints_button_clicked(GtkWidget* pWidget, gpointer pData)
{
	// ********** Déclarations (locales) ************
	char pcharMessage[50+1]="",pcharStringConcat[255+1]="",pcharAffEndPoints[255+1]="";
	char pcharStrUAconnect[30+1];
    GtkTextBuffer* pTextbuffer;
	GtkCssProvider* pProvider;
	GtkStyleContext* pContext;
	// *********** Fin de déclarations **************

	// Récupère le buffer de la zone de texte
	pTextbuffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(pBrowseObj_TextView));
	// Mise en forme et styles
	pProvider=gtk_css_provider_new();
	gtk_css_provider_load_from_data(pProvider,"textview{font: 14 courier new;font-weight:bold;}",-1,NULL);
	pContext=gtk_widget_get_style_context(pBrowseObj_TextView);
	gtk_style_context_add_provider(pContext,GTK_STYLE_PROVIDER(pProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	// Vérifie la connexion
	if (Connection==Disconnected)
	{
		sprintf(pcharMessage,"Disconnected !");
		fMessage(pcharMessage);
		gtk_text_buffer_set_text(pTextbuffer,"",-1);
	}
	else
	{
		// Listing endpoints
		UA_EndpointDescription* endpointArray=NULL;
		size_t endpointArraySize=0;
		sprintf(pcharStrUAconnect,"opc.tcp://%s%s",pcharIPaddress,":4840");
		UA_StatusCode UAretval=UA_Client_getEndpoints(pPlc_OPCclient,pcharStrUAconnect,&endpointArraySize,&endpointArray);
		if(UAretval!=UA_STATUSCODE_GOOD)
		{
			UA_Array_delete(endpointArray,endpointArraySize,&UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
			UA_Client_delete(pPlc_OPCclient);
			sprintf(pcharMessage,"OPC UA server not responding status = %i", UAretval);
			fMessage(pcharMessage);
			return;
		}
		sprintf(pcharStringConcat,"\n%i endpoints found\n",(int)endpointArraySize-2);
		strcat(pcharAffEndPoints,pcharStringConcat);
		for(size_t i=0;i<endpointArraySize-2;i++)
		{
			sprintf(pcharStringConcat,"URL of endpoint %i is %.*s\n", (int)i,(int)endpointArray[i].endpointUrl.length,endpointArray[i].endpointUrl.data);
			strcat(pcharAffEndPoints,pcharStringConcat);
		}
		gtk_text_buffer_set_text(pTextbuffer,pcharAffEndPoints,-1);
		UA_Array_delete(endpointArray,endpointArraySize,&UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
	}
}


// Bouton browse objects
G_MODULE_EXPORT void on_Browse_button_clicked(GtkWidget* pWidget, gpointer pData)
{
	// ********** Déclarations (locales) ************
	char pcharMessage[50+1]="",pcharStringConcat[255+1]="",pcharAffObjects[255+1]="";
    UA_BrowseRequest bReq;
    GtkTextBuffer* pTextbuffer;
	GtkCssProvider* pProvider;
	GtkStyleContext* pContext;
	// *********** Fin de déclarations **************

	// Récupère le buffer de la zone de texte
	pTextbuffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(pBrowseObj_TextView));
	// Mise en forme et styles
	pProvider=gtk_css_provider_new();
	gtk_css_provider_load_from_data(pProvider,"textview{font: 14 courier new;font-weight:bold;}",-1,NULL);
	pContext=gtk_widget_get_style_context(pBrowseObj_TextView);
	gtk_style_context_add_provider(pContext,GTK_STYLE_PROVIDER(pProvider),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	// Vérifie la connexion
	if (Connection==Disconnected)
	{
		sprintf(pcharMessage,"Disconnected !");
		fMessage(pcharMessage);
		gtk_text_buffer_set_text(pTextbuffer,"",-1);
	}
	else
	{
		UA_BrowseRequest_init(&bReq);
		bReq.requestedMaxReferencesPerNode = 0;
		bReq.nodesToBrowse = UA_BrowseDescription_new();
		bReq.nodesToBrowseSize = 1;
		bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); // browse objects folder
		bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; // return everything
		UA_BrowseResponse bResp = UA_Client_Service_browse(pPlc_OPCclient, bReq);
		sprintf(pcharStringConcat,"\n%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");
		strcat(pcharAffObjects,pcharStringConcat);
		for(size_t i = 0; i < bResp.resultsSize; ++i) 
		{
			for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) 
			{
				UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
				if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) 
				{
					sprintf(pcharStringConcat,"%-9d %-16d %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
						   ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
						   ref->browseName.name.data, (int)ref->displayName.text.length,
						   ref->displayName.text.data);
					strcat(pcharAffObjects,pcharStringConcat);
				} else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) 
				{
					sprintf(pcharStringConcat,"%-9d %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
						   (int)ref->nodeId.nodeId.identifier.string.length,
						   ref->nodeId.nodeId.identifier.string.data,
						   (int)ref->browseName.name.length, ref->browseName.name.data,
						   (int)ref->displayName.text.length, ref->displayName.text.data);
					strcat(pcharAffObjects,pcharStringConcat);
				}
				// TODO: distinguish further types
			}
		}
		gtk_text_buffer_set_text(pTextbuffer,pcharAffObjects,-1);
		
		UA_BrowseRequest_deleteMembers(&bReq);
		UA_BrowseResponse_deleteMembers(&bResp);
	}
}


// Bouton d'écriture
G_MODULE_EXPORT void on_write_button_clicked(GtkWidget* pWidget, gpointer pData)
{
	// ********** Déclarations (locales) ************
	char pcharMessage[50+1]="";
	const char* pcharAddrEntry;
	const char* pcharValueEntry;
	const char* pcharNameSpaceEntry;
	const char* pcharNameEntry;
	char* pcharType;
	int RegAddress;
	int RegValue;
	int WriteResponse;
	sfEisIntFloat EntryParser;
	gboolean MB_OPC_RadioButton;
	UA_StatusCode UAretval;
	UA_UInt16 Namespace;
	UA_Boolean UA_bValueToWrite;
	UA_Int16 UA_iValueToWrite;
	UA_Double UA_dValueToWrite;
	UA_String UA_sValueToWrite;
	UA_Variant vValueToWrite; // Un type variant et un "conteneur" dans lequel peut se trouver n'importe quel type
	// *********** Fin de déclarations **************

	// Récupère le contenu alphanumérique des zones de saisies
	pcharAddrEntry=gtk_entry_get_text(GTK_ENTRY(pRegister_address_entry));
	pcharValueEntry=gtk_entry_get_text(GTK_ENTRY(pWrite_value));
	pcharNameSpaceEntry=gtk_entry_get_text(GTK_ENTRY(pNamespace_entry));
	pcharNameEntry=gtk_entry_get_text(GTK_ENTRY(pName_entry));
	// Récupère l'état du groupe de boutons radio Modbus/OPC
	MB_OPC_RadioButton=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pOPC_radiobutton));
	// Récupère le texte actif du ComboBoxText
	pcharType=gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(pType_comboBoxText));

	// Vérifie la connexion
	if (Connection==Disconnected)
	{
		sprintf(pcharMessage,"Disconnected !");
		fMessage(pcharMessage);
	}
	else
	{
		if (!MB_OPC_RadioButton)
		// Modbus TCP
		{
			// Analyse les caractères saisis : adresse du premier registre
			EntryParser.pcharEntry=pcharAddrEntry;
			EntryParser.type=typeInteger;
			EntryParser.min=0; // limite basse
			EntryParser.max=1024; // limite haute
			fEntryIsIntOrFloat(&EntryParser);
			if (EntryParser.errNumber!=0)
			{
				sprintf(pcharMessage,EntryParser.pcharErrMessage);
				fMessage(pcharMessage);
				return;
			}
			else
			{
				sprintf(pcharMessage," ");
				fMessage(pcharMessage);
				RegAddress=EntryParser.intResult;
			}

			// Analyse les caractères saisis : valeur à écrire dans le registre
			EntryParser.pcharEntry=pcharValueEntry;
			EntryParser.type=typeInteger;
			EntryParser.min=0; // limite basse
			EntryParser.max=9999; // limite haute
			fEntryIsIntOrFloat(&EntryParser);
			if (EntryParser.errNumber!=0)
			{
				sprintf(pcharMessage,EntryParser.pcharErrMessage);
				fMessage(pcharMessage);
				return;
			}
			else
			{
				sprintf(pcharMessage," ");
				fMessage(pcharMessage);
				RegValue=EntryParser.intResult;
			}
			
			// Ecriture du premier registre (code 6) à l'adresse indiquée
			WriteResponse=modbus_write_register(pPlc_MBclient, RegAddress, RegValue);

			// La commande modbus a échoué
			if (WriteResponse==-1)
			{
				sprintf(pcharMessage,"Socket error = %d", WSAGetLastError());
				fMessage(pcharMessage);
			}
		}
		else
		// OPC UA
		{	
			// Analyse les caractères saisis : namespace
			EntryParser.pcharEntry=pcharNameSpaceEntry;
			EntryParser.type=typeInteger;
			EntryParser.min=1; // limite basse
			EntryParser.max=10; // limite haute
			fEntryIsIntOrFloat(&EntryParser);
			if (EntryParser.errNumber!=0)
			{
				sprintf(pcharMessage,EntryParser.pcharErrMessage);
				fMessage(pcharMessage);
				return;
			}
			else
			{
				sprintf(pcharMessage," ");
				fMessage(pcharMessage);
				Namespace=EntryParser.intResult;
			}

			/* Write node value attribute */
			UA_Variant_init(&vValueToWrite);
			
			if (!strcmp(pcharType,"bool"))
			{
				// Analyse les caractères saisis : valeur à écrire
				EntryParser.pcharEntry=pcharValueEntry;
				EntryParser.type=typeInteger;
				EntryParser.min=0; // limite basse
				EntryParser.max=1; // limite haute
				fEntryIsIntOrFloat(&EntryParser);
				if (EntryParser.errNumber!=0)
				{
					sprintf(pcharMessage,EntryParser.pcharErrMessage);
					fMessage(pcharMessage);
					return;
				}
				else
				{
					sprintf(pcharMessage," ");
					fMessage(pcharMessage);
					UA_bValueToWrite=(UA_Boolean)EntryParser.intResult;
				}
				
				UA_Variant_setScalar(&vValueToWrite,&UA_bValueToWrite,&UA_TYPES[UA_TYPES_BOOLEAN]); // Charge une valeur scalaire (alphanumérique) dans le variant
				UAretval=UA_Client_writeValueAttribute(pPlc_OPCclient,UA_NODEID_STRING(Namespace,(char*)pcharNameEntry),&vValueToWrite);

				if(UAretval!=UA_STATUSCODE_GOOD)
				{
					sprintf(pcharMessage,"Write attribute failed !");
					fMessage(pcharMessage);
				}
			}
			if (!strcmp(pcharType,"int"))
			{
				// Analyse les caractères saisis : valeur à écrire
				EntryParser.pcharEntry=pcharValueEntry;
				EntryParser.type=typeInteger;
				EntryParser.min=-32768; // limite basse
				EntryParser.max=32767; // limite haute
				fEntryIsIntOrFloat(&EntryParser);
				if (EntryParser.errNumber!=0)
				{
					sprintf(pcharMessage,EntryParser.pcharErrMessage);
					fMessage(pcharMessage);
					return;
				}
				else
				{
					sprintf(pcharMessage," ");
					fMessage(pcharMessage);
					UA_iValueToWrite=(UA_Int16)EntryParser.intResult;
				}
				
				UA_Variant_setScalar(&vValueToWrite,&UA_iValueToWrite,&UA_TYPES[UA_TYPES_INT16]); // Charge une valeur scalaire (alphanumérique) dans le variant
				UAretval=UA_Client_writeValueAttribute(pPlc_OPCclient,UA_NODEID_STRING(Namespace,(char*)pcharNameEntry),&vValueToWrite);

				if(UAretval!=UA_STATUSCODE_GOOD)
				{
					sprintf(pcharMessage,"Write attribute failed !");
					fMessage(pcharMessage);
				}
			}
			if (!strcmp(pcharType,"lreal"))
			{
				// Analyse les caractères saisis : valeur à écrire
				EntryParser.pcharEntry=pcharValueEntry;
				EntryParser.type=typeFloat;
				EntryParser.min=-DBL_MAX; // limite basse
				EntryParser.max=DBL_MAX; // limite haute
				fEntryIsIntOrFloat(&EntryParser);
				if (EntryParser.errNumber!=0)
				{
					sprintf(pcharMessage,EntryParser.pcharErrMessage);
					fMessage(pcharMessage);
					return;
				}
				else
				{
					sprintf(pcharMessage," ");
					fMessage(pcharMessage);
					UA_dValueToWrite=(UA_Double)EntryParser.doubleResult;
				}
				
				UA_Variant_setScalar(&vValueToWrite,&UA_dValueToWrite,&UA_TYPES[UA_TYPES_DOUBLE]); // Charge une valeur scalaire (alphanumérique) dans le variant
				UAretval=UA_Client_writeValueAttribute(pPlc_OPCclient,UA_NODEID_STRING(Namespace,(char*)pcharNameEntry),&vValueToWrite);

				if(UAretval!=UA_STATUSCODE_GOOD)
				{
					sprintf(pcharMessage,"Write attribute failed !");
					fMessage(pcharMessage);
				}
			}
			if (!strcmp(pcharType,"string"))
			{
				UA_sValueToWrite.data=(UA_Byte*)pcharValueEntry;
				
				UA_Variant_setScalar(&vValueToWrite,&UA_sValueToWrite,&UA_TYPES[UA_TYPES_STRING]); // Charge une valeur scalaire (alphanumérique) dans le variant
				UAretval=UA_Client_writeValueAttribute(pPlc_OPCclient,UA_NODEID_STRING(Namespace,(char*)pcharNameEntry),&vValueToWrite);

				if(UAretval!=UA_STATUSCODE_GOOD)
				{
					sprintf(pcharMessage,"Write attribute failed !");
					fMessage(pcharMessage);
				}
			}
		}
	}
}


/**********************************************
 ********** Fonctions Applicatives ************
 **********************************************
*/

// Affiche un message dans le bandeau inférieur
void fMessage(char* pcharMessage)
{	
	// ********** Déclarations (locales) ************
	
	// *********** Fin de déclarations **************

	
	// Affiche le message	
	gtk_label_set_text(GTK_LABEL(pMessages_display),pcharMessage);
}

// Lecture périodique
gboolean fPeriodicRead(gpointer pData)
{
	// ********** Déclarations (locales) ************
	gboolean Periodic;
	// *********** Fin de déclarations **************

	// Récupère l'état de la case à cocher
	Periodic=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pPeriodicRead_CheckButton));

	if (Periodic)
	{
		fRead();
	}
	
	return TRUE; // FALSE : appel périodique arrêté
}

// Lecture de registre(s)
void fRead()
{
	// ********** Déclarations (locales) ************
	char pcharMessage[50+1]="",pcharString1[32+1]="",pcharAffReadValue[256+1]="",pcharCleanedString[256+1]="";	
	const char* pcharAddrEntry;
	const char* pcharNbOfRegEntry;
	const char* pcharNameSpaceEntry;
	const char* pcharNameEntry;
	char* pcharType;
	char* pcharBoolState;
	int RegAddress,NbOfReg,loop,ReadResponse;
	UA_UInt16 Namespace;
	uint16_t RegValues[10];
	GtkTextBuffer* pTextbuffer;
	sfEisIntFloat EntryParser;
	gboolean MB_OPC_RadioButton;
	UA_Boolean valueBool;
	UA_Int16 valueInt;
	UA_Double valueLreal;
	UA_String valueString;
	// *********** Fin de déclarations **************


	// Récupère le contenu alphanumérique des zones de saisies
	pcharAddrEntry=gtk_entry_get_text(GTK_ENTRY(pRegister_address_entry));
	pcharNbOfRegEntry=gtk_entry_get_text(GTK_ENTRY(pNumber_of_Registers_entry));
	pcharNameSpaceEntry=gtk_entry_get_text(GTK_ENTRY(pNamespace_entry));
	pcharNameEntry=gtk_entry_get_text(GTK_ENTRY(pName_entry));
	// Récupère le buffer de la zone de texte
	pTextbuffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(pRegValues_TextView));
	// Récupère l'état du groupe de boutons radio Modbus/OPC
	MB_OPC_RadioButton=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pOPC_radiobutton));
	// Récupère le texte actif du ComboBoxText
	pcharType=gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(pType_comboBoxText));


	// Vérifie la connexion
	if (Connection==Disconnected)
	{
		sprintf(pcharMessage,"Disconnected !");
		fMessage(pcharMessage);
	}
	else
	{
		if (!MB_OPC_RadioButton)
		// Modbus TCP
		{
			// Analyse les caractères saisis : adresse du premier registre
			EntryParser.pcharEntry=pcharAddrEntry;
			EntryParser.type=typeInteger;
			EntryParser.min=0; // limite basse
			EntryParser.max=1024; // limite haute
			fEntryIsIntOrFloat(&EntryParser);
			if (EntryParser.errNumber!=0)
			{
				sprintf(pcharMessage,EntryParser.pcharErrMessage);
				fMessage(pcharMessage);
				return;
			}
			else
			{
				sprintf(pcharMessage," ");
				fMessage(pcharMessage);
				RegAddress=EntryParser.intResult;
			}
			
			// Analyse les caractères saisis : nombre de registres
			EntryParser.pcharEntry=pcharNbOfRegEntry;
			EntryParser.type=typeInteger;
			EntryParser.min=1; // limite basse
			EntryParser.max=10; // limite haute
			fEntryIsIntOrFloat(&EntryParser);
			if (EntryParser.errNumber!=0)
			{
				sprintf(pcharMessage,EntryParser.pcharErrMessage);
				fMessage(pcharMessage);
				return;
			}
			else
			{
				sprintf(pcharMessage," ");
				fMessage(pcharMessage);
				NbOfReg=EntryParser.intResult;
			}

			// Lecture de registre(s) (code 3) à partir de l'adresse indiquée
			ReadResponse=modbus_read_registers(pPlc_MBclient, RegAddress, NbOfReg, RegValues);
				// La commande modbus a échoué
			if (ReadResponse==-1)
			{
				sprintf(pcharMessage,"Socket error = %d", WSAGetLastError());
				fMessage(pcharMessage);
			}
			else
			{
				// Affiche la ou les valeur(s)
				gtk_text_buffer_set_text(pTextbuffer,"",-1);
				for (loop = 0; loop < NbOfReg; loop++)
				{
					sprintf(pcharString1,"Value=%d(0x%x)\r\n",RegValues[loop],RegValues[loop]);
					strcat(pcharAffReadValue,pcharString1);
				}
				gtk_text_buffer_set_text(pTextbuffer,pcharAffReadValue,-1);
			}
		}
		else
		// OPC UA
		{
			// Analyse les caractères saisis : namespace
			EntryParser.pcharEntry=pcharNameSpaceEntry;
			EntryParser.type=typeInteger;
			EntryParser.min=1; // limite basse
			EntryParser.max=10; // limite haute
			fEntryIsIntOrFloat(&EntryParser);
			if (EntryParser.errNumber!=0)
			{
				sprintf(pcharMessage,EntryParser.pcharErrMessage);
				fMessage(pcharMessage);
				return;
			}
			else
			{
				sprintf(pcharMessage," ");
				fMessage(pcharMessage);
				Namespace=EntryParser.intResult;
			}
			
			/* Read node value attribute */
			UA_Variant* pVariantValueToRead=UA_Variant_new();
			UA_StatusCode UAretval=UA_Client_readValueAttribute(pPlc_OPCclient,UA_NODEID_STRING(Namespace,(char*)pcharNameEntry),pVariantValueToRead);

			if (!strcmp(pcharType,"bool"))
			{
				if(UAretval==UA_STATUSCODE_GOOD && UA_Variant_isScalar(pVariantValueToRead) && pVariantValueToRead->type==&UA_TYPES[UA_TYPES_BOOLEAN])
				{
					valueBool=*(UA_Boolean*)pVariantValueToRead->data;
					gtk_text_buffer_set_text(pTextbuffer,"",-1);
					if (valueBool)
					{
						pcharBoolState="TRUE";
					}
					else
					{
						pcharBoolState="FALSE";
					}
					sprintf(pcharAffReadValue,"State=%s",pcharBoolState);
					gtk_text_buffer_set_text(pTextbuffer,pcharAffReadValue,-1);
				}
				else
				{
					sprintf(pcharMessage,"Read attribute failed !");
					fMessage(pcharMessage);
				}
			}
			else if (!strcmp(pcharType,"int"))
			{
				if(UAretval==UA_STATUSCODE_GOOD && UA_Variant_isScalar(pVariantValueToRead) && pVariantValueToRead->type==&UA_TYPES[UA_TYPES_INT16])
				{
					valueInt=*(UA_Int16*)pVariantValueToRead->data;
					gtk_text_buffer_set_text(pTextbuffer,"",-1);
					sprintf(pcharAffReadValue,"Value=%d(0x%x)",valueInt,valueInt);
					gtk_text_buffer_set_text(pTextbuffer,pcharAffReadValue,-1);
				}
				else
				{
					sprintf(pcharMessage,"Read attribute failed !");
					fMessage(pcharMessage);
				}
			}
			else if (!strcmp(pcharType,"lreal"))
			{
				if(UAretval==UA_STATUSCODE_GOOD && UA_Variant_isScalar(pVariantValueToRead) && pVariantValueToRead->type==&UA_TYPES[UA_TYPES_DOUBLE])
				{
					valueLreal=*(UA_Double*)pVariantValueToRead->data;
					gtk_text_buffer_set_text(pTextbuffer,"",-1);
					sprintf(pcharAffReadValue,"Value=%f",valueLreal);
					gtk_text_buffer_set_text(pTextbuffer,pcharAffReadValue,-1);
				}
				else
				{
					sprintf(pcharMessage,"Read attribute failed !");
					fMessage(pcharMessage);
				}
			}
			else if (!strcmp(pcharType,"string"))
			{
				if(UAretval==UA_STATUSCODE_GOOD && UA_Variant_isScalar(pVariantValueToRead) && pVariantValueToRead->type==&UA_TYPES[UA_TYPES_STRING])
				{
					valueString=*(UA_String*)pVariantValueToRead->data;
					gtk_text_buffer_set_text(pTextbuffer,"",-1);
					strncpy(pcharCleanedString,(const char*)valueString.data,valueString.length);
					sprintf(pcharAffReadValue,"Contents=%s\nLength=%i",pcharCleanedString,valueString.length);
					gtk_text_buffer_set_text(pTextbuffer,pcharAffReadValue,-1);
				}
				else
				{
					sprintf(pcharMessage,"Read attribute failed !");
					fMessage(pcharMessage);
				}
			}
			UA_Variant_delete(pVariantValueToRead);				
		}
	}
}

