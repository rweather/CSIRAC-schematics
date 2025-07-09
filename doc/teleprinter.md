CSIRAC Teleprinter and Flexowriter Codes
========================================

Both the original teleprinter and the Flexowriter use 5-bit codes
with "Figure Shift" and "Letter Shift" characters to select between
two sets of 32 codes.

The original teleprinter had letters, digits, greek letters, and a
number of punctuation symbols.  It was clearly intended for use with
mathematical programs.  The Flexowriter is a little closer to ASCII
with symbols more suitable for programming languages.

The emulator converts between these codes and plain text in the
UTF-8 encoding.

The emulator also has an "ASCII teletype" mode that allows writing
programs that input and output ASCII characters.  If the CSIRAC project
had continued, it is entirely possible that the engineers would have
eventually connected a newfangled ASCII teletype to the machine.

<table border="1">
<tr><td><b>Code</b></td><td><b>Teleprinter<br/>Letter Shift</b></td><td><b>Teleprinter<br/>Figure Shift</b></td><td><b>Flexowriter<br/>Letter Shift</b</td><td><b>Flexowriter<br/>Figure Shift</b></td></tr>
<tr><td>0</td><td>A</td><td>0</td><td>Blank <sup>1</sup</td><td>Blank <sup>1</sup</td></tr>
<tr><td>1</td><td>B</td><td>1</td><td>Q</td><td>1</td></tr>
<tr><td>2</td><td>C</td><td>2</td><td>W</td><td>2</td></tr>
<tr><td>3</td><td>D</td><td>3</td><td>C</td><td>*</td></tr>
<tr><td>4</td><td>E</td><td>4</td><td>R</td><td>4</td></tr>
<tr><td>5</td><td>F</td><td>5</td><td>K</td><td>(</td></tr>
<tr><td>6</td><td>G</td><td>6</td><td>L</td><td>)</td></tr>
<tr><td>7</td><td>H</td><td>7</td><td>U</td><td>7</td></tr>
<tr><td>8</td><td>I</td><td>8</td><td>I</td><td>8</td></tr>
<tr><td>9</td><td>J</td><td>9</td><td>D</td><td>#</td></tr>
<tr><td>10</td><td>K</td><td>+</td><td>V</td><td>=</td></tr>
<tr><td>11</td><td>L</td><td>-</td><td>A</td><td>-</td></tr>
<tr><td>12</td><td>M</td><td>.</td><td>F</td><td>&amp;</td></tr>
<tr><td>13</td><td>N</td><td>)</td><td>M</td><td>.</td></tr>
<tr><td>14</td><td>O</td><td>(</td><td>G</td><td>TAB</td></tr>
<tr><td>15</td><td>P</td><td>i</td><td>N</td><td>,</td></tr>
<tr><td>16</td><td>Q</td><td>j</td><td>P</td><td>0</td></tr>
<tr><td>17</td><td>R</td><td>k</td><td>J</td><td>STOP</td></tr>
<tr><td>18</td><td>S</td><td>&#x2207;</td><td>H</td><td>&#163;</td></tr>
<tr><td>19</td><td>T</td><td>&#x03D5;</td><td>E</td><td>3</td></tr>
<tr><td>20</td><td>U</td><td>&#x03C8;</td><td>B</td><td>'</td></tr>
<tr><td>21</td><td>V</td><td>&#x03B8;</td><td>T</td><td>5</td></tr>
<tr><td>22</td><td>W</td><td>&#x03A9;</td><td>Y</td><td>6</td></tr>
<tr><td>23</td><td>X</td><td>&#x0393;</td><td>S</td><td>/</td></tr>
<tr><td>24</td><td>Y</td><td>&#x03C0;</td><td>X</td><td>x</td></tr>
<tr><td>25</td><td>Z</td><td>&#x03A3;</td><td>O</td><td>9</td></tr>
<tr><td>26</td><td>&#x039B;</td><td>&#x039E;</td><td>Z</td><td>+</td></tr>
<tr><td>27</td><td>Figure Shift</td><td>Figure Shift</td><td>Figure Shift</td><td>Figure Shift</td></tr>
<tr><td>28</td><td>Letter Shift</td><td>Letter Shift</td><td>Space</td><td>Space</td></tr>
<tr><td>29</td><td>Line Feed</td><td>Line Feed</td><td>Carriage Return</td><td>Carriage Return</td></tr>
<tr><td>30</td><td>Carriage Return</td><td>Carriage Return</td><td>Letter Shift</td><td>Letter Shift</td></tr>
<tr><td>31</td><td>Space</td><td>Space</td><td>Line Feed <sup>2</sup></td><td>Line Feed <sup>2</sup></td></tr>
</table>

Note 1: The Flexowriter "Blank" is different from a space, and indicates
that the tape position has not been punched with data at all.

Note 2: The Flexowrtier "Line Feed" is used as an "erase" character by the
CSIRAC on input.  It corresponds to punching all 5 holes on a tape position
which "erases" whatever was there before.
