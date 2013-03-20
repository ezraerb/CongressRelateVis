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
    updates. Please contact me through LinkedIn or github (my profile
    also has a link to the code depository)
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

// Standard technique to allow multiple header inclusion (needed due to being a subclass
#ifndef XML_PARSER_DEFINE
#define XML_PARSER_DEFINE

#include<fstream>

using std::string;
using std::ifstream;
using std::cerr;
using std::endl;

class XmlParser
{
public:
	~XmlParser(); // Closes any open files

    // Sets the directory to find XML files
    void setFileDirectory(const string& directory);

	/* Turns on tracing of file parsing. Data returned is the buffer and
        index values, after every method call. */
    void setTrace(void);

protected:
    /* Constructor. Sets internal state. Protected to prevent using this class
        outside a subclass that does the actual XML parsing. */
    XmlParser();

    // Opens a specific XML file. Returns true if succeeded, else false
	bool open(const string& filename);

    // Closes the current XML file, if any, and clears object state
	void close(void);

    // Returns true if a file is currently open
    bool isOpen(void);

    // Returns true if every line of the file has been read
	bool haveEOF(void);

    // Returns true if the end of a line has been reached
	bool haveLineEnd(void);

    // Returns true if there is still text to process.
    bool haveTextToProcess(void);

    /* Burns file text until a given XML key string is found. If the key
        is not found, burns to either the end of the current line or EOF.
        If burnKey is set, it also burns the key, so the file is set to
        the first character AFTER the key, otherwise the file will be the
        first character OF the key */
	void burnToKey(const string& key, bool burnToEOF = true, bool burnKey = true);

    /* Burns a given number of chars in the current line of the file.
        Anything except a newline counts as text. This method stops
        at the end of the line. Returns true if the burn was complete,
        false if the end of line was reached early or the file is at EOF */
	bool burnChars(unsigned int charCount);

    /* Extracts the given number of chars from the current line, and
        then burns them. The rules are the same as char burning. If
        the line runs out early, the size of the result will be shorter
        than the request */
	string getText(unsigned int charCount);

    /* Extracts chars from the current text up to (but not including) the
        passed token, and then burns them. The rules are the same as char
        burning. If the token does not exist the return value is all data
        to the end of the line. If the client needs to check that the token
        exists, call haveLineEnd() after this routine (remember that the
        token is not included in the burn) */
	string getTextToToken(char token);

    /* Extracts a number from the current text from the current position to
        the passed token. The token must exist. Returns true if a number
        was read, else false The text before the token is burned regardless */
    bool getNumberToToken(char token, int& number);

private:
  std::ifstream _file; // Actual file
  string _buffer; // Line from file currently being processed
  double _nxtSrchPos; // Next char in the buffer to process


  // Flag to dump the file parsing data to standard error
  bool _parseTrace;

  // File directory
  string _fileDirectory;

	/* Internal method to reload the internal buffer with the next line of
        the file. */
    void reloadBuffer(void);

    // Resets object state for a new file
    void clearFileData(void);

    // Outputs a header used for tracing purposes
    void outputPositionHeader(void);
};

inline void XmlParser::outputPositionHeader(void)
{
    cerr << "01234567890123456789012345678901234567890123456789012345678901234567890123456789" << endl;
}

inline void XmlParser::setTrace(void)
{
    /* Normally, the header is output when a file is open. If a file is already
        open, need to do it here instead */
    if (isOpen())
        outputPositionHeader();
    _parseTrace = true;
}

// Sets the directory to find XML files
inline void XmlParser::setFileDirectory(const string& directory)
{
    _fileDirectory = directory;
    /* The passed directory does not include the backslash needed before the filename,
        so add it
        TRICKY NOTE: Notice the double backslash below. C++ uses '\' as an
        escape character. The first is the esacpe character needed to insert a
        litteral '\' in the string! */
    _fileDirectory.append("\\");
}

inline void XmlParser::close(void)
{
    _file.close();
    clearFileData();
}

inline bool XmlParser::isOpen(void)
{
    return _file.is_open();
}

// Returns true if the end of the file processing has been reached
inline bool XmlParser::haveEOF(void)
{
    /* This class reads unformatted input, so the end of the file or a fatal
        error indicates end of processing */
    if (!isOpen())
        return true;
    else
        return (_file.bad() || _file.eof());
}

// Returns true if the end of a line has been reached
inline bool XmlParser::haveLineEnd(void)
{
    return (_nxtSrchPos == string::npos);
}

// Returns true if there is still text to process.
inline bool XmlParser::haveTextToProcess(void)
{
    /* Still have text if current line is not finished, OR
        the file still has lines to reaad */
    return ((!haveEOF()) || (!haveLineEnd()));
}


inline bool XmlParser::open(const string& fileName)
{
    if (_file.is_open())
        close();
    // Closing the file invalidates the processing data

    string fullFilePath(_fileDirectory);
    fullFilePath.append(fileName);
    // Trace the full file path, to catch the error where the directory is wrong
    if (_parseTrace)
        cerr << "File to open: " << fullFilePath << endl;
    _file.open(fullFilePath.c_str());
    // New file, so output the trace header
    if (_file.is_open() && _parseTrace)
        outputPositionHeader();
    return _file.is_open();
}

// Resets object state for a new file
inline void XmlParser::clearFileData(void)
{
  _buffer = "";
  _nxtSrchPos = string::npos;
}

#endif // Header not already included
