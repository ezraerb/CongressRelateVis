/* This file is part of CongressRelationVis. This project creates graphs
   showing simmilarity of voting of different members of the US House of
   Representatives.

    Copyright (C) 2013   Ezra Erb

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as published
    by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    I'd appreciate a note if you find this program useful or make
    updates. Please contact me through LinkedIn or github (my profile also has
    a link to the code depository)
*/

/* This base class implements a very specialized XML parser. It contains
    infrastructure for managing files, finding keywords, and extracting
    data based on those keys. Subclasses are responsible for the details
    of what data to extract, and processing it. The file data is
    encapsulated in this routine so subclasses can’t corrupt it.

	This class was chosen over a full XML library because only a low number
	of tags are processed, and they appear in a defined order in well-formed
	files. Searching for them specifically should be more efficient.
	*/
#include<fstream>
#include<string>
#include<sstream>
#include<iostream>
#include<algorithm>
#include"xmlParser.h"

/* NASTY HACK: Hardcode the file path to the roll call files. This should
    really be in a configuration file, but will work for this program */
#define ROLL_DIRECTORY "GovTrackData"

using std::string;
using std::ifstream;
using std::istringstream;
using std::cerr;
using std::endl;

/* Constructor. Note that this is protected to prevent using this class being
   used outside a subclass that does the actual XML parsing. */

XmlParser::XmlParser() : _file(), _buffer(), _fileDirectory()
{
   _parseTrace = false;
   clearFileData();
   // HACK: Set the file directoty to a hard coded directory. Should get from configuration
   setFileDirectory(ROLL_DIRECTORY);
}

XmlParser::~XmlParser()
{
    _file.close();
}

// Private inline methods must be declared before they are called
/* Internal method to reload the internal buffer with the next line of
    the file. This exists to get around C++ limitation of loading a file line
    by line with formatted input (it can only be done with unformatted reads) */
inline void XmlParser::reloadBuffer(void)
{
    if (haveEOF()) {
        // Can't extract anything, so set to empty string
        string temp;
        _buffer = temp;
    }
    else {
        getline(_file, _buffer);
        if (_parseTrace)
            cerr << _buffer << endl;
    }
    if (_buffer.empty())
        _nxtSrchPos = string::npos; // Nothing to process
    else
        _nxtSrchPos = 0;
}

/* Burns file text until a given XML key string is found. If the key
    is not found, burns to either the end of the current line or EOF.
    If burnKey is set, it also burns the key, so the file is set to
    the first character AFTER the key, otherwise the file will be the
    first character OF the key */
void XmlParser::burnToKey(const string& key, bool burnToEOF, bool burnKey)
{
    if (!isOpen())
        return; // Nothing to process!
    else if (!haveTextToProcess())
        return; // Current line is burned, and no more lines to read
    // Check remainder of current line first
    if (!haveLineEnd())
        _nxtSrchPos = _buffer.find(key, _nxtSrchPos + 1);
    while ((!haveEOF()) && haveLineEnd() && (burnToEOF || (!_buffer.size()))) {
        // Read and check more lines
        reloadBuffer();
        // Reading the last line of the file sets EOF; still need to check the buffer
        _nxtSrchPos = _buffer.find(key, _nxtSrchPos + 1);
    } // While lines left in the file and reason to read them
    if (!haveLineEnd()) { // Found something
        if (_parseTrace)
            cerr << "Found key " << key << " position " << _nxtSrchPos << endl;
        if (burnKey)
            // The entire key was found by the find() call, so can skip over it without a length check
            _nxtSrchPos += key.length();
    }
}

/* Burns a given number of chars in the current line of the file. Anything except
   a newline counts as text. This method stops at the end of the line. Returns true
   if the burn was complete, false if the end of line was reached early or the file
   is at EOF */
bool XmlParser::burnChars(unsigned int charCount)
{
    if (!haveTextToProcess())
        return false;

    if (_parseTrace)
        cerr << "Burn " << charCount << " chars starting at " << _nxtSrchPos << endl;

    if ((_nxtSrchPos + charCount) > _buffer.length()) {
        // Buffer not long enough
        _nxtSrchPos = string::npos;
        return false;
    }
    else {
        _nxtSrchPos += charCount;
        // Check for the case where the burn uses up the buffer
        if (_nxtSrchPos >= _buffer.length())
            _nxtSrchPos = string::npos;
        return true;
    }
}

/* Extracts the given number of chars from the current line, and then burns
    them. The rules are the same as char burning. If the line runs out early,
    the size of the result will be shorter than the request */
string XmlParser::getText(unsigned int charCount)
{
    // Extract text, then burn
    string result;
    if (haveTextToProcess() && (charCount > 0)) {
        if (_parseTrace)
            cerr << "Extract " << charCount << " chars starting at " << _nxtSrchPos;
        result = string(_buffer, _nxtSrchPos, charCount);
        if (_parseTrace)
            // Since whitespace can be important for parsing, show the result between '
            cerr << ":'" << result << "'" << endl;
        burnChars(charCount);
    }
    return result;
}

/* Extracts chars from the current text up to (but not including) the passed
   token, and then burns them. The rules are the same as char burning. If the
   token does not exist the return value is all data to the end of the line.
   If the client needs to check that the token exists, call haveLineEnd() after
   this routine (remember that the token is NOT included in the burn) */
string XmlParser::getTextToToken(char token)
{
    if (!haveTextToProcess()) {
        string result;
        return result;
    }
    // Find the position of the char
    unsigned int charPos = _buffer.find_first_of(token, _nxtSrchPos);

    if (_parseTrace)
        cerr << "Get to Token:" << token << " located at " << charPos << endl;

    // If the next character IS the token, nothing to extract
    if (charPos == _nxtSrchPos) {
        string result;
        return result;
    }

    // If the token is not found, set the final position to one beyond end
    if (charPos == string::npos)
        charPos = _buffer.length() + 1;
    return getText(charPos - _nxtSrchPos);
}

/* Extracts a number from the current text from the current position to
    the passed token. The token must exist. Returns true if a number
    was read, else false. The text before the token is burned regardless */
bool XmlParser::getNumberToToken(char token, int& number)
{
    istringstream temp(getTextToToken(token));
    bool success;
    if (haveLineEnd())
        success = false;
    else {
        // C++ method of converting a string to a short
        temp >> number;

        /* There are other methods of getting this result (including the ! operator!)
            but they are really just cryptic wrappers around the statement below */
        success = !temp.fail();
    }
    if (!success)
        number = 0; // Ensure consistent behavior
    return success;
}

