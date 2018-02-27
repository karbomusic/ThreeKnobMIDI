/*
 
  ## RGB Color Table for ThreeKnobMidiController ##
  
  The 1st dimension of the array is the vertical (entire color row)
  The 2nd dimension contains the three RGB values that make up that color
  The colors enum matches the color postions of the 1st dimension in the color table.

  Note: Values are derived with sub 100r resistor on GND.
        The better method is to calculate using 1 resister per color.
        This will be added in a later update but essentially it means the
        RGB values will not need to be tweaked like they are below.
        
 */

const static int colorTable[9][3]  = {
  {127, 255, 250},   /*  0 white   */
  {133, 128, 0},     /*  1 orange  */
  {128, 0, 255},     /*  2 purple  */
  {0, 200 , 50},     /*  3 cyan    */
  {120, 255, 20},    /*  4 yellow  */
  {128, 0, 0},       /*  5 red     */
  {0, 0, 255},       /*  6 blue    */
  {0, 128, 0},       /*  7 green   */
  {0, 0, 0}          /*  8 none    */
};

 enum colors
{
  white = 0,
  orange = 1,
  purple = 2,
  cyan = 3,
  yellow = 4,
  red = 5,
  blue = 6,
  green = 7,
  none = 8
};
