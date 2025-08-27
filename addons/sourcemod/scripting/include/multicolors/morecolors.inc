// MOAR COLORS
// By Dr. McKay
// Inspired by: https://forums.alliedmods.net/showthread.php?t=96831

#if defined _more_colors_included
 #endinput
#endif
#define _more_colors_included

#pragma newdecls optional

#define MORE_COLORS_VERSION     "2.0.0-MC"
#define MC_MAX_MESSAGE_LENGTH   256
#define MAX_BUFFER_LENGTH       (MC_MAX_MESSAGE_LENGTH * 4)
#define MAX_COLOR_TAG_LENGTH 32
#define COLOR_HASH_SIZE 256

#define MCOLOR_RED              0xFF4040
#define MCOLOR_BLUE             0x99CCFF
#define MCOLOR_GRAY             0xCCCCCC
#define MCOLOR_GREEN            0x3EFF3E

#define MC_GAME_DODS           0

bool MC_SkipList[MAXPLAYERS+1];
StringMap MC_Trie;
int MC_TeamColors[][] = {{0xCCCCCC, 0x4D7942, 0xFF4040}}; // Multi-dimensional array for games that don't support SayText2. First index is the game index (as defined by the GAME_ defines), second index is team. 0 = spectator, 1 = team1, 2 = team2

static ConVar sm_show_activity;

/**
 * Prints a message to a specific client in the chat area.
 * Supports color tags.
 * 
 * @param client		Client index.
 * @param message		Message (formatting rules).
 * 
 * On error/Errors:		If the client is not connected an error will be thrown.
 */
stock void MC_PrintToChat(int client, const char[] message, any ...) {
	MC_CheckTrie();

	if (client <= 0 || client > MaxClients) {
		ThrowError("Invalid client index %i", client);
	}

	if (!IsClientInGame(client)) {
		ThrowError("Client %i is not in game", client);
	}

	char buffer[MAX_BUFFER_LENGTH];
	char buffer2[MAX_BUFFER_LENGTH];

	SetGlobalTransTarget(client);
	Format(buffer, sizeof(buffer), "\x01%s", message);
	VFormat(buffer2, sizeof(buffer2), buffer, 3);

	MC_ReplaceColorCodes(buffer2);
	MC_SendMessage(client, buffer2);
}

/**
 * Prints a message to all clients in the chat area.
 * Supports color tags.
 * 
 * @param client		Client index.
 * @param message		Message (formatting rules).
 */
stock void MC_PrintToChatAll(const char[] message, any ...) {
	MC_CheckTrie();

	char buffer[MAX_BUFFER_LENGTH], buffer2[MAX_BUFFER_LENGTH];

	for (int i = 1; i <= MaxClients; ++i) {
		if (!IsClientInGame(i) || MC_SkipList[i]) {
			MC_SkipList[i] = false;
			continue;
		}

		SetGlobalTransTarget(i);
		Format(buffer, sizeof(buffer), "\x01%s", message);
		VFormat(buffer2, sizeof(buffer2), buffer, 2);

		MC_ReplaceColorCodes(buffer2);
		MC_SendMessage(i, buffer2);
	}
}

/**
 * Prints a message to a specific client in the chat area.
 * Supports color tags and teamcolor tag.
 * 
 * @param client		Client index.
 * @param author		Author index whose color will be used for teamcolor tag.
 * @param message		Message (formatting rules).
 * 
 * On error/Errors:		If the client or author are not connected an error will be thrown
 */
stock void MC_PrintToChatEx(int client, int author, const char[] message, any ...) {
	MC_CheckTrie();

	if (client <= 0 || client > MaxClients) {
		ThrowError("Invalid client index %i", client);
	}

	if (!IsClientInGame(client)) {
		ThrowError("Client %i is not in game", client);
	}

	if (author < 0 || author > MaxClients) {
		ThrowError("Invalid client index %i", author);
	}

	if ((author != 0) && !IsClientInGame(author)) {
		ThrowError("Client %i is not in game", author);
	}

	char buffer[MAX_BUFFER_LENGTH], buffer2[MAX_BUFFER_LENGTH];
	SetGlobalTransTarget(client);
	Format(buffer, sizeof(buffer), "\x01%s", message);
	VFormat(buffer2, sizeof(buffer2), buffer, 4);
	MC_ReplaceColorCodes(buffer2, author);
	MC_SendMessage(client, buffer2, author);
}

/**
 * Prints a message to all clients in the chat area.
 * Supports color tags and teamcolor tag.
 *
 * @param author	  Author index whose color will be used for teamcolor tag.
 * @param message   Message (formatting rules).
 * 
 * On error/Errors:   If the author is not connected an error will be thrown.
 */
stock void MC_PrintToChatAllEx(int author, const char[] message, any ...) {
	MC_CheckTrie();

	if (author < 0 || author > MaxClients) {
		ThrowError("Invalid client index %i", author);
	}

	if ((author != 0) && !IsClientInGame(author)) {
		ThrowError("Client %i is not in game", author);
	}

	char buffer[MAX_BUFFER_LENGTH];
	char buffer2[MAX_BUFFER_LENGTH];

	for (int i = 1; i <= MaxClients; ++i) {
		if (!IsClientInGame(i) || MC_SkipList[i]) {
			MC_SkipList[i] = false;
			continue;
		}

		SetGlobalTransTarget(i);
		Format(buffer, sizeof(buffer), "\x01%s", message);
		VFormat(buffer2, sizeof(buffer2), buffer, 3);

		MC_ReplaceColorCodes(buffer2, author);
		MC_SendMessage(i, buffer2, author);
	}
}

/**
 * Sends a SayText2 usermessage
 * 
 * @param client	Client to send usermessage to
 * @param message	Message to send
 */
stock void MC_SendMessage(int client, const char[] message, int author = 0) {
	if (author == 0) {
		author = client;
	}

	char buffer[MC_MAX_MESSAGE_LENGTH];
	strcopy(buffer, sizeof(buffer), message);

	UserMsg index = GetUserMessageId("SayText2");
	if (index == INVALID_MESSAGE_ID) {
		if (GetEngineVersion() == Engine_DODS) {
			int team = GetClientTeam(author);
			if (team == 0) {
				ReplaceString(buffer, sizeof(buffer), "\x03", "\x04", false); // Unassigned gets green
			}
			else {
				char temp[16];
				Format(temp, sizeof(temp), "\x07%06X", MC_TeamColors[MC_GAME_DODS][team - 1]);
				ReplaceString(buffer, sizeof(buffer), "\x03", temp, false);
			}
		}

		PrintToChat(client, "%s", buffer);
		return;
	}

	Handle buf = StartMessageOne("SayText2", client, USERMSG_RELIABLE|USERMSG_BLOCKHOOKS);
	if (GetFeatureStatus(FeatureType_Native, "GetUserMessageType") == FeatureStatus_Available && GetUserMessageType() == UM_Protobuf) {
		Protobuf pb = UserMessageToProtobuf(buf);
		pb.SetInt("ent_idx", author);
		pb.SetBool("chat", true);
		pb.SetString("msg_name", buffer);
		pb.AddString("params", "");
		pb.AddString("params", "");
		pb.AddString("params", "");
		pb.AddString("params", "");
	}
	else {
		BfWrite bf = UserMessageToBfWrite(buf);
		bf.WriteByte(author); // Message author
		bf.WriteByte(true); // Chat message
		bf.WriteString(buffer); // Message text
	}

	EndMessage();
}

/**
 * This function should only be used right in front of
 * MC_PrintToChatAll or MC_PrintToChatAllEx. It causes those functions
 * to skip the specified client when printing the message.
 * After printing the message, the client will no longer be skipped.
 * 
 * @param client   Client index
 */
stock void MC_SkipNextClient(int client) {
	if (client <= 0 || client > MaxClients) {
		ThrowError("Invalid client index %i", client);
	}
	
	MC_SkipList[client] = true;
}

/**
 * Checks if the colors trie is initialized and initializes it if it's not (used internally)
 * 
 * @return			No return
 */
stock void MC_CheckTrie() {
	if (MC_Trie == null) {
		MC_Trie = MC_InitColorTrie();
	}
}

/**
 * Replaces color tags in a string with color codes (used internally by MC_PrintToChat, MC_PrintToChatAll, MC_PrintToChatEx, and MC_PrintToChatAllEx
 *
 * @param buffer		String.
 * @param author		Optional client index to use for {teamcolor} tags, or 0 for none
 * @param removeTags	Optional boolean value to determine whether we're replacing tags with colors, or just removing tags, used by MC_RemoveTags
 * @param maxlen		Optional value for max buffer length, used by MC_RemoveTags
 * 
 * On error/Errors:		If the client index passed for author is invalid or not in game.
 */
stock void MC_ReplaceColorCodes(char[] buffer, int author = 0, bool removeTags = false, int maxlen = MAX_BUFFER_LENGTH) {
	MC_InitFastColors();
	if (!removeTags) {
		FastReplaceString(buffer, maxlen, "{default}", "\x01");

		if (author != 0) {
			if (author < 0 || author > MaxClients) {
				ThrowError("Invalid client index %i", author);
			}

			if (!IsClientInGame(author)) {
				ThrowError("Client %i is not in game", author);
			}

			FastReplaceString(buffer, maxlen, "{teamcolor}", "\x03");
		}
	}
	else {
		FastReplaceString(buffer, maxlen, "{default}", "");
		FastReplaceString(buffer, maxlen, "{teamcolor}", "");
	}

	int outPos = 0;
	int len = strlen(buffer);
	int i = 0;

	while (i < len && outPos < maxlen - 16) {
		if (buffer[i] == '{') {
			int start = i + 1;
			int end = -1;

			// Find closing brace
			for (int j = start; j < len && j < start + MAX_COLOR_TAG_LENGTH; j++) {
				if (buffer[j] == '}') {
					end = j;
					break;
				}
			}

			if (end != -1) {
				int tagLen = end - start;

				// Extract tag
				int copyLen = (tagLen < 31) ? tagLen : 31;
				for (int k = 0; k < copyLen; k++) {
					g_sTempTag[k] = buffer[start + k];
				}
				g_sTempTag[copyLen] = '\0';

				MC_StrToLower(g_sTempTag);

				char value[16];
				bool found = false;

				// Handle hex colors
				if (g_sTempTag[0] == '#' && (tagLen == 7 || tagLen == 9)) {
					if (tagLen == 7) {
						value[0] = '\x07';
						strcopy(value[1], 9, g_sTempTag[1]);
					} else {
						value[0] = '\x08';
						strcopy(value[1], 9, g_sTempTag[1]);
					}
					found = true;
				} else {
					found = FastGetColor(g_sTempTag, value, sizeof(value));
				}

				if (found && !removeTags) {
					// Copy color code directly
					int valLen = strlen(value);
					for (int k = 0; k < valLen && outPos < maxlen - 1; k++) {
						g_sOutputBuffer[outPos++] = value[k];
					}
					i = end + 1;
					continue;
				} else if (removeTags) {
					i = end + 1;
					continue;
				}
			}
		}

		g_sOutputBuffer[outPos++] = buffer[i++];
	}

	g_sOutputBuffer[outPos] = '\0';
	strcopy(buffer, maxlen, g_sOutputBuffer);
}

/**
 * Gets a part of a string
 * 
 * @param input			String to get the part from
 * @param output		Buffer to write to
 * @param maxlen		Max length of output buffer
 * @param start			Position to start at
 * @param numChars		Number of characters to return, or 0 for the end of the string
 */
stock void CSubString(const char[] input, char[] output, int maxlen, int start, int numChars = 0) {
	int i = 0;
	for (;;) {
		if (i == maxlen - 1 || i >= numChars || input[start + i] == '\0') {
			output[i] = '\0';
			return;
		}

		output[i] = input[start + i];
		i++;
	}
}

/**
 * Converts a string to lowercase
 * 
 * @param buffer		String to convert
 */
stock void MC_StrToLower(char[] buffer) {
	for (int i = 0; buffer[i]; i++) {
		if (buffer[i] >= 'A' && buffer[i] <= 'Z') {
			buffer[i] |= 0x20;
		}
	}
}

/**
 * Adds a color to the colors trie
 *
 * @param name			Color name, without braces
 * @param color			Hexadecimal representation of the color (0xRRGGBB)
 * @return				True if color was added successfully, false if a color already exists with that name
 */
stock bool MC_AddColor(const char[] name, int color) {
	MC_InitFastColors();

	char value[16];
	char newName[64];
	strcopy(newName, sizeof(newName), name);
	MC_StrToLower(newName);

	if (FastGetColor(newName, value, sizeof(value))) {
		return false;
	}

	char colorValue[16];
	Format(colorValue, sizeof(colorValue), "\x07%06X", color);
	MC_FastSetColor(newName, colorValue);
	return true;
}

/**
 * Removes color tags from a message
 * 
 * @param message		Message to remove tags from
 * @param maxlen		Maximum buffer length
 */
stock void MC_RemoveTags(char[] message, int maxlen) {
	MC_ReplaceColorCodes(message, 0, true, maxlen);
}

/**
 * Replies to a command with colors
 * 
 * @param client		Client to reply to
 * @param message		Message (formatting rules)
 */
stock void MC_ReplyToCommand(int client, const char[] message, any ...) {
	char buffer[MAX_BUFFER_LENGTH];
	SetGlobalTransTarget(client);
	VFormat(buffer, sizeof(buffer), message, 3);

	if (client == 0) {
		MC_RemoveTags(buffer, sizeof(buffer));
		PrintToServer("%s", buffer);
	}
	else if (GetCmdReplySource() == SM_REPLY_TO_CONSOLE) {
		MC_RemoveTags(buffer, sizeof(buffer));
		PrintToConsole(client, "%s", buffer);
	}
	else {
		MC_PrintToChat(client, "%s", buffer);
	}
}

/**
 * Replies to a command with colors
 * 
 * @param client		Client to reply to
 * @param author		Client to use for {teamcolor}
 * @param message		Message (formatting rules)
 */
stock void MC_ReplyToCommandEx(int client, int author, const char[] message, any ...) {
	char buffer[MAX_BUFFER_LENGTH];
	SetGlobalTransTarget(client);
	VFormat(buffer, sizeof(buffer), message, 4);

	if (client == 0) {
		MC_RemoveTags(buffer, sizeof(buffer));
		PrintToServer("%s", buffer);
	}
	else if (GetCmdReplySource() == SM_REPLY_TO_CONSOLE) {
		MC_RemoveTags(buffer, sizeof(buffer));
		PrintToConsole(client, "%s", buffer);
	}
	else {
		MC_PrintToChatEx(client, author, "%s", buffer);
	}
}

/**
 * Displays usage of an admin command to users depending on the 
 * setting of the sm_show_activity cvar.  
 *
 * This version does not display a message to the originating client 
 * if used from chat triggers or menus.  If manual replies are used 
 * for these cases, then this function will suffice.  Otherwise, 
 * MC_ShowActivity2() is slightly more useful.
 * Supports color tags.
 *
 * @param client		Client index doing the action, or 0 for server.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @error
 */
stock int MC_ShowActivity(int client, const char[] format, any ...) {
	if (sm_show_activity == null) {
		sm_show_activity = FindConVar("sm_show_activity");
	}

	char tag[] = "[SM] ";

	char szBuffer[MC_MAX_MESSAGE_LENGTH];
	//char szCMessage[MC_MAX_MESSAGE_LENGTH];
	int value = sm_show_activity.IntValue;
	ReplySource replyto = GetCmdReplySource();

	char name[MAX_NAME_LENGTH] = "Console";
	char sign[MAX_NAME_LENGTH] = "ADMIN";
	bool display_in_chat = false;
	if (client != 0) {
		if (client < 0 || client > MaxClients || !IsClientConnected(client))
			ThrowError("Client index %d is invalid", client);

		GetClientName(client, name, sizeof(name));
		AdminId id = GetUserAdmin(client);
		if (id == INVALID_ADMIN_ID || !id.HasFlag(Admin_Generic, Access_Effective)) {
			sign = "PLAYER";
		}

		/* Display the message to the client? */
		if (replyto == SM_REPLY_TO_CONSOLE) {
			SetGlobalTransTarget(client);
			VFormat(szBuffer, sizeof(szBuffer), format, 3);

			MC_RemoveTags(szBuffer, sizeof(szBuffer));
			PrintToConsole(client, "%s%s", tag, szBuffer);
			display_in_chat = true;
		}
	}
	else {
		SetGlobalTransTarget(LANG_SERVER);
		VFormat(szBuffer, sizeof(szBuffer), format, 3);

		MC_RemoveTags(szBuffer, sizeof(szBuffer));
		PrintToServer("%s%s", tag, szBuffer);
	}

	if (!value) {
		return 1;
	}

	for (int i = 1; i <= MaxClients; ++i) {
		if (!IsClientInGame(i) || IsFakeClient(i) || (display_in_chat && i == client)) {
			continue;
		}

		AdminId id = GetUserAdmin(i);
		SetGlobalTransTarget(i);
		if (id == INVALID_ADMIN_ID || !id.HasFlag(Admin_Generic, Access_Effective)) {
			/* Treat this as a normal user. */
			if ((value & 1) | (value & 2)) {
				char newsign[MAX_NAME_LENGTH];

				if ((value & 2) || (i == client)) {
					newsign = name;
				}
				else {
					newsign = sign;
				}

				VFormat(szBuffer, sizeof(szBuffer), format, 3);

				MC_PrintToChatEx(i, client, "%s%s: %s", tag, newsign, szBuffer);
			}
		}
		else {
			/* Treat this as an admin user */
			bool is_root = id.HasFlag(Admin_Root, Access_Effective);
			if ((value & 4) || (value & 8) || ((value & 16) && is_root)) {
				char newsign[MAX_NAME_LENGTH];

				if ((value & 8) || ((value & 16) && is_root) || (i == client)) {
					newsign = name;
				}
				else {
					newsign = sign;
				}

				VFormat(szBuffer, sizeof(szBuffer), format, 3);

				MC_PrintToChatEx(i, client, "%s%s: %s", tag, newsign, szBuffer);
			}
		}
	}

	return 1;
}

/**
 * Same as MC_ShowActivity(), except the tag parameter is used instead of "[SM] " (note that you must supply any spacing).
 * Supports color tags.
 *
 * @param client		Client index doing the action, or 0 for server.
 * @param tags			Tag to display with.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @error
 */
stock int MC_ShowActivityEx(int client, const char[] tag, const char[] format, any ...) {
	if (sm_show_activity == null) {
		sm_show_activity = FindConVar("sm_show_activity");
	}

	char szTag[MC_MAX_MESSAGE_LENGTH];
	strcopy(szTag, sizeof(szTag), tag);
	MC_RemoveTags(szTag, sizeof(szTag));

	char szBuffer[MC_MAX_MESSAGE_LENGTH];
	//char szCMessage[MC_MAX_MESSAGE_LENGTH];
	int value = sm_show_activity.IntValue;
	ReplySource replyto = GetCmdReplySource();

	char name[MAX_NAME_LENGTH] = "Console";
	char sign[MAX_NAME_LENGTH] = "ADMIN";
	bool display_in_chat = false;
	if (client != 0) {
		if (client < 0 || client > MaxClients || !IsClientConnected(client)) {
			ThrowError("Client index %d is invalid", client);
		}

		GetClientName(client, name, sizeof(name));
		AdminId id = GetUserAdmin(client);
		if (id == INVALID_ADMIN_ID || !id.HasFlag(Admin_Generic, Access_Effective)) {
			sign = "PLAYER";
		}

		/* Display the message to the client? */
		if (replyto == SM_REPLY_TO_CONSOLE) {
			SetGlobalTransTarget(client);
			VFormat(szBuffer, sizeof(szBuffer), format, 4);

			MC_RemoveTags(szBuffer, sizeof(szBuffer));
			PrintToConsole(client, "%s%s", szTag, szBuffer);
			display_in_chat = true;
		}
	}
	else {
		SetGlobalTransTarget(LANG_SERVER);
		VFormat(szBuffer, sizeof(szBuffer), format, 4);

		MC_RemoveTags(szBuffer, sizeof(szBuffer));
		PrintToServer("%s%s", szTag, szBuffer);
	}

	if (!value) {
		return 1;
	}

	for (int i = 1; i <= MaxClients; ++i) {
		if (!IsClientInGame(i) || IsFakeClient(i) || (display_in_chat && i == client)) {
			continue;
		}

		AdminId id = GetUserAdmin(i);
		SetGlobalTransTarget(i);
		if (id == INVALID_ADMIN_ID || !id.HasFlag(Admin_Generic, Access_Effective)) {
			/* Treat this as a normal user. */
			if ((value & 1) | (value & 2)) {
				char newsign[MAX_NAME_LENGTH];

				if ((value & 2) || (i == client)) {
					newsign = name;
				}
				else {
					newsign = sign;
				}

				VFormat(szBuffer, sizeof(szBuffer), format, 4);

				MC_PrintToChatEx(i, client, "%s%s: %s", tag, newsign, szBuffer);
			}
		}
		else {
			/* Treat this as an admin user */
			bool is_root = id.HasFlag(Admin_Root, Access_Effective);
			if ((value & 4) || (value & 8) || ((value & 16) && is_root)) {
				char newsign[MAX_NAME_LENGTH];

				if ((value & 8) || ((value & 16) && is_root) || (i == client)) {
					newsign = name;
				}
				else {
					newsign = sign;
				}

				VFormat(szBuffer, sizeof(szBuffer), format, 4);

				MC_PrintToChatEx(i, client, "%s%s: %s", tag, newsign, szBuffer);
			}
		}
	}

	return 1;
}

/**
 * Displays usage of an admin command to users depending on the setting of the sm_show_activity cvar.
 * All users receive a message in their chat text, except for the originating client, 
 * who receives the message based on the current ReplySource.
 * Supports color tags.
 *
 * @param client		Client index doing the action, or 0 for server.
 * @param tags			Tag to prepend to the message.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @error
 */
stock int MC_ShowActivity2(int client, const char[] tag, const char[] format, any ...) {
	if (sm_show_activity == null) {
		sm_show_activity = FindConVar("sm_show_activity");
	}

	char szTag[MC_MAX_MESSAGE_LENGTH];
	strcopy(szTag, sizeof(szTag), tag);
	MC_RemoveTags(szTag, sizeof(szTag));

	char szBuffer[MC_MAX_MESSAGE_LENGTH];
	//char szCMessage[MC_MAX_MESSAGE_LENGTH];
	int value = sm_show_activity.IntValue;
	// ReplySource replyto = GetCmdReplySource();

	char name[MAX_NAME_LENGTH] = "Console";
	char sign[MAX_NAME_LENGTH] = "ADMIN";

	if (client != 0) {
		if (client < 0 || client > MaxClients || !IsClientConnected(client)) {
			ThrowError("Client index %d is invalid", client);
		}

		GetClientName(client, name, sizeof(name));

		AdminId id = GetUserAdmin(client);
		if (id == INVALID_ADMIN_ID || !id.HasFlag(Admin_Generic, Access_Effective)) {
			sign = "PLAYER";
		}

		SetGlobalTransTarget(client);
		VFormat(szBuffer, sizeof(szBuffer), format, 4);

		/* We don't display directly to the console because the chat text 
		 * simply gets added to the console, so we don't want it to print 
		 * twice.
		 */
		MC_PrintToChatEx(client, client, "%s%s", szTag, szBuffer);
	}
	else {
		SetGlobalTransTarget(LANG_SERVER);
		VFormat(szBuffer, sizeof(szBuffer), format, 4);

		MC_RemoveTags(szBuffer, sizeof(szBuffer));
		PrintToServer("%s%s", szTag, szBuffer);
	}

	if (!value) {
		return 1;
	}

	for (int i = 1; i <= MaxClients; ++i) {
		if (!IsClientInGame(i) || IsFakeClient(i) || i == client) {
			continue;
		}

		AdminId id = GetUserAdmin(i);
		SetGlobalTransTarget(i);
		if (id == INVALID_ADMIN_ID || !id.HasFlag(Admin_Generic, Access_Effective)) {
			/* Treat this as a normal user. */
			if ((value & 1) | (value & 2)) {
				char newsign[MAX_NAME_LENGTH];

				if ((value & 2)) {
					newsign = name;
				}
				else {
					newsign = sign;
				}

				VFormat(szBuffer, sizeof(szBuffer), format, 4);

				MC_PrintToChatEx(i, client, "%s%s: %s", tag, newsign, szBuffer);
			}
		}
		else {
			/* Treat this as an admin user */
			bool is_root = id.HasFlag(Admin_Root, Access_Effective);
			if ((value & 4) || (value & 8) || ((value & 16) && is_root)) {
				char newsign[MAX_NAME_LENGTH];
				

				if ((value & 8) || ((value & 16) && is_root)) {
					newsign = name;
				}
				else {
					newsign = sign;
				}

				VFormat(szBuffer, sizeof(szBuffer), format, 4);

				MC_PrintToChatEx(i, client, "%s%s: %s", tag, newsign, szBuffer);
			}
		}
	}

	return 1;
}

/**
 * Determines whether a color name exists
 * 
 * @param color			The color name to check
 * @return				True if the color exists, false otherwise
 */
stock bool CColorExists(const char[] color) {
	MC_InitFastColors();
	char temp[16];
	char colorName[64];
	strcopy(colorName, sizeof(colorName), color);
	MC_StrToLower(colorName);
	return MC_FastGetColor(colorName, temp, sizeof(temp));
}

/**
 * Returns the hexadecimal representation of a client's team color (will NOT initialize the trie)
 *
 * @param client		Client to get the team color for
 * @return				Client's team color in hexadecimal, or green if unknown
 * On error/Errors:		If the client index passed is invalid or not in game.
 */
stock int CGetTeamColor(int client) {
	if (client <= 0 || client > MaxClients) {
		ThrowError("Invalid client index %i", client);
	}

	if (!IsClientInGame(client)) {
		ThrowError("Client %i is not in game", client);
	}

	int value;
	switch(GetClientTeam(client)) {
		case 1: {
			value = MCOLOR_GRAY;
		}
		case 2: {
			value = MCOLOR_RED;
		}
		case 3: {
			value = MCOLOR_BLUE;
		}
		default: {
			value = MCOLOR_GREEN;
		}
	}

	return value;
}

stock void MC_InitFastColors() {
	if (g_iColorCount > 0)
		return;

	MC_FastSetColor("aliceblue", "0xF0F8FF");
	MC_FastSetColor("allies", "0x4D7942"); // same as Allies team in DoD:S
	MC_FastSetColor("ancient", "0xEB4B4B"); // same as Ancient item rarity in Dota 2
	MC_FastSetColor("antiquewhite", "0xFAEBD7");
	MC_FastSetColor("aqua", "0x00FFFF");
	MC_FastSetColor("aquamarine", "0x7FFFD4");
	MC_FastSetColor("arcana", "0xADE55C"); // same as Arcana item rarity in Dota 2
	MC_FastSetColor("axis", "0xFF4040"); // same as Axis team in DoD:S
	MC_FastSetColor("azure", "0x007FFF");
	MC_FastSetColor("beige", "0xF5F5DC");
	MC_FastSetColor("bisque", "0xFFE4C4");
	MC_FastSetColor("black", "0x000000");
	MC_FastSetColor("blanchedalmond", "0xFFEBCD");
	MC_FastSetColor("blue", "0x99CCFF"); // same as BLU/Counter-Terrorist team color
	MC_FastSetColor("blueviolet", "0x8A2BE2");
	MC_FastSetColor("brown", "0xA52A2A");
	MC_FastSetColor("burlywood", "0xDEB887");
	MC_FastSetColor("cadetblue", "0x5F9EA0");
	MC_FastSetColor("chartreuse", "0x7FFF00");
	MC_FastSetColor("chocolate", "0xD2691E");
	MC_FastSetColor("collectors", "0xAA0000"); // same as Collector's item quality in TF2
	MC_FastSetColor("common", "0xB0C3D9"); // same as Common item rarity in Dota 2
	MC_FastSetColor("community", "0x70B04A"); // same as Community item quality in TF2
	MC_FastSetColor("coral", "0xFF7F50");
	MC_FastSetColor("cornflowerblue", "0x6495ED");
	MC_FastSetColor("cornsilk", "0xFFF8DC");
	MC_FastSetColor("corrupted", "0xA32C2E"); // same as Corrupted item quality in Dota 2
	MC_FastSetColor("crimson", "0xDC143C");
	MC_FastSetColor("cyan", "0x00FFFF");
	MC_FastSetColor("darkblue", "0x00008B");
	MC_FastSetColor("darkcyan", "0x008B8B");
	MC_FastSetColor("darkgoldenrod", "0xB8860B");
	MC_FastSetColor("darkgray", "0xA9A9A9");
	MC_FastSetColor("darkgrey", "0xA9A9A9");
	MC_FastSetColor("darkgreen", "0x006400");
	MC_FastSetColor("darkkhaki", "0xBDB76B");
	MC_FastSetColor("darkmagenta", "0x8B008B");
	MC_FastSetColor("darkolivegreen", "0x556B2F");
	MC_FastSetColor("darkorange", "0xFF8C00");
	MC_FastSetColor("darkorchid", "0x9932CC");
	MC_FastSetColor("darkred", "0x8B0000");
	MC_FastSetColor("darksalmon", "0xE9967A");
	MC_FastSetColor("darkseagreen", "0x8FBC8F");
	MC_FastSetColor("darkslateblue", "0x483D8B");
	MC_FastSetColor("darkslategray", "0x2F4F4F");
	MC_FastSetColor("darkslategrey", "0x2F4F4F");
	MC_FastSetColor("darkturquoise", "0x00CED1");
	MC_FastSetColor("darkviolet", "0x9400D3");
	MC_FastSetColor("deeppink", "0xFF1493");
	MC_FastSetColor("deepskyblue", "0x00BFFF");
	MC_FastSetColor("dimgray", "0x696969");
	MC_FastSetColor("dimgrey", "0x696969");
	MC_FastSetColor("dodgerblue", "0x1E90FF");
	MC_FastSetColor("exalted", "0xCCCCCD"); // same as Exalted item quality in Dota 2
	MC_FastSetColor("firebrick", "0xB22222");
	MC_FastSetColor("floralwhite", "0xFFFAF0");
	MC_FastSetColor("forestgreen", "0x228B22");
	MC_FastSetColor("frozen", "0x4983B3"); // same as Frozen item quality in Dota 2
	MC_FastSetColor("fuchsia", "0xFF00FF");
	MC_FastSetColor("fullblue", "0x0000FF");
	MC_FastSetColor("fullred", "0xFF0000");
	MC_FastSetColor("gainsboro", "0xDCDCDC");
	MC_FastSetColor("genuine", "0x4D7455"); // same as Genuine item quality in TF2
	MC_FastSetColor("ghostwhite", "0xF8F8FF");
	MC_FastSetColor("gold", "0xFFD700");
	MC_FastSetColor("goldenrod", "0xDAA520");
	MC_FastSetColor("gray", "0xCCCCCC"); // same as spectator team color
	MC_FastSetColor("grey", "0xCCCCCC");
	MC_FastSetColor("green", "0x3EFF3E");
	MC_FastSetColor("greenyellow", "0xADFF2F");
	MC_FastSetColor("haunted", "0x38F3AB"); // same as Haunted item quality in TF2
	MC_FastSetColor("honeydew", "0xF0FFF0");
	MC_FastSetColor("hotpink", "0xFF69B4");
	MC_FastSetColor("immortal", "0xE4AE33"); // same as Immortal item rarity in Dota 2
	MC_FastSetColor("indianred", "0xCD5C5C");
	MC_FastSetColor("indigo", "0x4B0082");
	MC_FastSetColor("ivory", "0xFFFFF0");
	MC_FastSetColor("khaki", "0xF0E68C");
	MC_FastSetColor("lavender", "0xE6E6FA");
	MC_FastSetColor("lavenderblush", "0xFFF0F5");
	MC_FastSetColor("lawngreen", "0x7CFC00");
	MC_FastSetColor("legendary", "0xD32CE6"); // same as Legendary item rarity in Dota 2
	MC_FastSetColor("lemonchiffon", "0xFFFACD");
	MC_FastSetColor("lightblue", "0xADD8E6");
	MC_FastSetColor("lightcoral", "0xF08080");
	MC_FastSetColor("lightcyan", "0xE0FFFF");
	MC_FastSetColor("lightgoldenrodyellow", "0xFAFAD2");
	MC_FastSetColor("lightgray", "0xD3D3D3");
	MC_FastSetColor("lightgrey", "0xD3D3D3");
	MC_FastSetColor("lightgreen", "0x99FF99");
	MC_FastSetColor("lightpink", "0xFFB6C1");
	MC_FastSetColor("lightsalmon", "0xFFA07A");
	MC_FastSetColor("lightseagreen", "0x20B2AA");
	MC_FastSetColor("lightskyblue", "0x87CEFA");
	MC_FastSetColor("lightslategray", "0x778899");
	MC_FastSetColor("lightslategrey", "0x778899");
	MC_FastSetColor("lightsteelblue", "0xB0C4DE");
	MC_FastSetColor("lightyellow", "0xFFFFE0");
	MC_FastSetColor("lime", "0x00FF00");
	MC_FastSetColor("limegreen", "0x32CD32");
	MC_FastSetColor("linen", "0xFAF0E6");
	MC_FastSetColor("magenta", "0xFF00FF");
	MC_FastSetColor("maroon", "0x800000");
	MC_FastSetColor("mediumaquamarine", "0x66CDAA");
	MC_FastSetColor("mediumblue", "0x0000CD");
	MC_FastSetColor("mediumorchid", "0xBA55D3");
	MC_FastSetColor("mediumpurple", "0x9370D8");
	MC_FastSetColor("mediumseagreen", "0x3CB371");
	MC_FastSetColor("mediumslateblue", "0x7B68EE");
	MC_FastSetColor("mediumspringgreen", "0x00FA9A");
	MC_FastSetColor("mediumturquoise", "0x48D1CC");
	MC_FastSetColor("mediumvioletred", "0xC71585");
	MC_FastSetColor("midnightblue", "0x191970");
	MC_FastSetColor("mintcream", "0xF5FFFA");
	MC_FastSetColor("mistyrose", "0xFFE4E1");
	MC_FastSetColor("moccasin", "0xFFE4B5");
	MC_FastSetColor("mythical", "0x8847FF"); // same as Mythical item rarity in Dota 2
	MC_FastSetColor("navajowhite", "0xFFDEAD");
	MC_FastSetColor("navy", "0x000080");
	MC_FastSetColor("normal", "0xB2B2B2"); // same as Normal item quality in TF2
	MC_FastSetColor("oldlace", "0xFDF5E6");
	MC_FastSetColor("olive", "0x9EC34F");
	MC_FastSetColor("olivedrab", "0x6B8E23");
	MC_FastSetColor("orange", "0xFFA500");
	MC_FastSetColor("orangered", "0xFF4500");
	MC_FastSetColor("orchid", "0xDA70D6");
	MC_FastSetColor("palegoldenrod", "0xEEE8AA");
	MC_FastSetColor("palegreen", "0x98FB98");
	MC_FastSetColor("paleturquoise", "0xAFEEEE");
	MC_FastSetColor("palevioletred", "0xD87093");
	MC_FastSetColor("papayawhip", "0xFFEFD5");
	MC_FastSetColor("peachpuff", "0xFFDAB9");
	MC_FastSetColor("peru", "0xCD853F");
	MC_FastSetColor("pink", "0xFFC0CB");
	MC_FastSetColor("plum", "0xDDA0DD");
	MC_FastSetColor("powderblue", "0xB0E0E6");
	MC_FastSetColor("purple", "0x800080");
	MC_FastSetColor("rare", "0x4B69FF"); // same as Rare item rarity in Dota 2
	MC_FastSetColor("red", "0xFF4040"); // same as RED/Terrorist team color
	MC_FastSetColor("rosybrown", "0xBC8F8F");
	MC_FastSetColor("royalblue", "0x4169E1");
	MC_FastSetColor("saddlebrown", "0x8B4513");
	MC_FastSetColor("salmon", "0xFA8072");
	MC_FastSetColor("sandybrown", "0xF4A460");
	MC_FastSetColor("seagreen", "0x2E8B57");
	MC_FastSetColor("seashell", "0xFFF5EE");
	MC_FastSetColor("selfmade", "0x70B04A"); // same as Self-Made item quality in TF2
	MC_FastSetColor("sienna", "0xA0522D");
	MC_FastSetColor("silver", "0xC0C0C0");
	MC_FastSetColor("skyblue", "0x87CEEB");
	MC_FastSetColor("slateblue", "0x6A5ACD");
	MC_FastSetColor("slategray", "0x708090");
	MC_FastSetColor("slategrey", "0x708090");
	MC_FastSetColor("snow", "0xFFFAFA");
	MC_FastSetColor("springgreen", "0x00FF7F");
	MC_FastSetColor("steelblue", "0x4682B4");
	MC_FastSetColor("strange", "0xCF6A32"); // same as Strange item quality in TF2
	MC_FastSetColor("tan", "0xD2B48C");
	MC_FastSetColor("teal", "0x008080");
	MC_FastSetColor("thistle", "0xD8BFD8");
	MC_FastSetColor("tomato", "0xFF6347");
	MC_FastSetColor("turquoise", "0x40E0D0");
	MC_FastSetColor("uncommon", "0xB0C3D9"); // same as Uncommon item rarity in Dota 2
	MC_FastSetColor("unique", "0xFFD700"); // same as Unique item quality in TF2
	MC_FastSetColor("unusual", "0x8650AC"); // same as Unusual item quality in TF2
	MC_FastSetColor("valve", "0xA50F79"); // same as Valve item quality in TF2
	MC_FastSetColor("vintage", "0x476291"); // same as Vintage item quality in TF2
	MC_FastSetColor("violet", "0xEE82EE");
	MC_FastSetColor("wheat", "0xF5DEB3");
	MC_FastSetColor("white", "0xFFFFFF");
	MC_FastSetColor("whitesmoke", "0xF5F5F5");
	MC_FastSetColor("yellow", "0xFFFF00");
	MC_FastSetColor("yellowgreen", "0x9ACD32");
}
