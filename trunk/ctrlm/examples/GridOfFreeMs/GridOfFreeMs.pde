/**
 * GridOfFreeMs -- Display images on a grid of FreeMs, 
 *                 using the new ColorSpot FreeM commands
 *                
 *
 * 2010, Tod E. Kurt, http://thingm.com/
 *
 * What controls do we need:
 * 1. Load image
 * 2. Send image to FreeMs
 * 3. Display image
 * 4. Dispaly image pixelized
 * 5. FreeM testing/setting grid
 * 6. Simple color picker for testing entire grid at once
 *
 *
 * Rough algorithm:
 * ----------------
 * A. Preparing the Image
 * 1. Read in image
 * 2. Resample to 10x10 grid
 * 3. 
 *
 * B. Loading the image onto FreeMs
 * 1. March through each pixel of 10x10 grid
 * 2. Set LinkM to apppropriate FreeM address
 * 3. Send SetColorSpot command to FreeM(s)
 *
 * C. Display image already on FreeMs
 * 1. Broadcast "change to color location #12" 
 * 2. There is no 2.
 *
 *
 */

import thingm.linkm.*;

import controlP5.*;

static boolean debug = true;

int gridw = 10;  // number of grid points wide
int gridh = 10;  // number of grid points tall

int dispw = 540;
int disph = 540;
int ctrlh = 100;  // height of controls
int btncnt = 9;
int btnx  = 20;
int btny  = 40;
int btnw  = 40;

int stridew;     // distance between "pixels" on screen width/gridw
int strideh;
int strideb = 10; // border between pixels

int dispMode = 0;
boolean uploading = false;

String setname = "gridset.txt";
String imgpath = "jelly.jpg";

ControlP5 controlP5;
Button btni[] = new Button[btncnt];
Textlabel status;
ColorPicker picker;
ControlWindow controlWindow;
Toggle dispToggle;
Toggle freemSet;


String imgf[] = new String[btncnt];
PImage imga[] = new PImage[btncnt];
PImage imgb[] = new PImage[btncnt];
int imgn = 0;  // current image index
PImage jellyaimg; // jellyfish standin
PImage jellybimg;

boolean isConnected = false;

LinkM linkm = new LinkM();
int ctrlmaddr;

// make Set the FreeMs popup window
void makeFreeMsWindow() {

  controlWindow = controlP5.addControlWindow("SetFreeMs",400,400,240,270);
  controlWindow.hide();
  controlWindow.hideCoordinates();
  controlWindow.setBackground(color(80));
  controlWindow.setTitle("Set the FreeMs!");

  Textlabel l = controlP5.addTextlabel("freemlbl1", 
                                       "Click on FreeM to view, or:", 20,20 );
  l.setWindow(controlWindow);

  freemSet = controlP5.addToggle("Set",  150, 17, 20,13);

  //freemSet.captionLabel().style().moveMargin(-14,20,0,0);
  freemSet.captionLabel().style().moveMargin(-14,0,0,3);
  freemSet.captionLabel().style().backgroundWidth = 46;

  freemSet.setWindow(controlWindow);
  
  for( int i=0; i<gridw; i++ ) {
    for( int j=0; j<gridh; j++ ) { 
      int c1 = 'A' + (char)i;
      int c2 = 'A' + (char)j;
      String s = ""+ (char)c2 +""+ (char)c1;
      Button b = controlP5.addButton(s, 1, 20 + (i*20), 50 + (j*20), 17,15);
      b.setWindow(controlWindow);
    }
  }

}

//
void setup() {
  size(dispw+20, ctrlh+disph+10);
  //size(dispw, ctrlh+disph, P2D);
  frameRate(20);
  //  smooth();

  stridew = dispw/gridw;
  strideh = disph/gridh;

  controlP5 = new ControlP5(this);
  controlP5.disableShortcuts();
  controlP5.setMoveable(false);

  //makeFreeMsWindow();

  controlP5.addButton("Load Img", 0, 10,10, 50,20 );

  controlP5.addButton("Load Set", 0, 100, 10, 50,20 );
  controlP5.addButton("Save Set", 0, 160, 10, 50,20 );

  controlP5.addButton("Set FreeMs", 0, 270,10, 60,20 );

  controlP5.addButton("All Off",  0, 400,10, 40,20 );
 
  picker = controlP5.addColorPicker("Pick", width-100,10,  50,30 );
  controlP5.addButton("Set All To", 0,      width-100,65,  50,15);

  controlP5.addButton("Send", 0, width-40, 10, 30,30 );
  dispToggle = controlP5.addToggle("Disp", false, width-40, 45, 30,30 );
  dispToggle.captionLabel().style().moveMargin(-22,0,0,5);
  status = controlP5.addTextlabel("status", "status: ready", 20,ctrlh-10 );

  // this seems inefficient
  for( int i=0; i< btncnt; i++ ) {
      int n = i+1;
      btni[i] = controlP5.addButton("i"+n,n, btnx+(i*(btnw+5)),btny, btnw,btnw);
      btni[i].setId( i + 100 );
  }

  // backup images
  jellyaimg = loadImage(imgpath);
  jellybimg = loadImage(imgpath);
  jellyaimg.resize(dispw,disph);
  jellybimg.resize(gridw,gridh);

  if( !loadSet(false) ) {
    exit();
    return;
  }

  connectIfNeeded();
  //status( "sketchPath: "+sketchPath);
}

//
void draw() {
  background( 100 );

  // FIXME: HACK to get border around selected image button
  fill(255,0,0);
  for( int i=0; i<btncnt; i++ ) { 
    if( i == imgn ) rect( btnx-3+(i*(btnw+5)),btny-3, btnw+6,btnw+6);
  }

  PImage a = imga[imgn];
  PImage b = imgb[imgn];
  //if( a==null ) a = jellyaimg;
  //if( b==null ) b = jellybimg;

  if( dispMode == 0 ) {             // display the original image
      image( a, 10,ctrlh, dispw,disph );
  } 
  else {                            // display downsampled image
    noStroke();
    for( int bx = 0; bx< b.width; bx++ ) {
      for( int by = 0; by< b.height; by++ ) {
        color bc = b.get(bx,by);
        fill( bc );
        rect( 15 + bx * stridew, 5 + ctrlh + (by * strideh), 
              stridew-strideb, strideh-strideb);
      }
    }
  }

  //controlP5.draw();
}

void status(String str) {
  str = "status: "+str;
  if(debug) println(str);
  status.setValue(str);
}


// Processing's callback
void keyPressed() {
  boolean shouldSend = dispToggle.getState();
  if( key == CODED ) { 
    if( keyCode == RIGHT ) {
      imgn++; if(imgn == btncnt ) imgn = 0;
      displayColors(shouldSend);
    }
    else if( keyCode == LEFT ) { 
      imgn--; if( imgn < 0 ) imgn = btncnt-1;;
      displayColors(shouldSend);
    }
  } 
  else if( key == ESC ) { 
    key = 0; // prevent PApplet from quitting us
    //chooseImage(0);
    uploading = false;
  }
  else if( key == ' ' ) {  // space bar
    dispMode = (dispMode==0) ? 1:0;
  } 
  else if( key == DELETE || key == BACKSPACE) { 
    isConnected = false;  // force reconnect
    connectIfNeeded();
  }
  else if( key >= '1' && key <= '9' ) {
    imgn = key-'1';
  }
}


// controlP5's callback
// used for handling the various buttons
public void controlEvent(ControlEvent e) {
  if( e.isGroup() ) {
    if(debug) println("group:"+e);
    return;
  }
  String name = e.controller().name();
  int id = e.controller().id();
  int val = (int)(e.controller().value());
  if(debug) println("ctrlEvent: "+name +":"+ id + ":"+ val);


  if( name.equals("Send") ) { 
    new Thread( new Uploader() ).start();
  } 
  else if( name.equals("Save Set") )  {
    status("Saving set to '"+setname+"'");
    saveSet();
  }
  else if( name.equals("Load Set") )  {
    if( loadSet(true) ) {
      status("Loaded set from '"+setname+"'");
    }
  }
  else if( name.equals("Load Img") ) { 
    chooseImage(imgn);
  }
  else if( name.equals("All Off") ) {
    status("turning all off");
    uploading = false;
    try { 
      linkm.ctrlmSetSendAddress( ctrlmaddr, 0, 0);  // quick, in ctrlm
      linkm.off(ctrlmaddr);
      linkm.setFadeSpeed(ctrlmaddr, 2);
    } catch(IOException ioe) {
      status("ioerror: "+ioe.getMessage());
    }
  }
  else if( name.equals("Set All To") ) {
    color c = picker.getColorValue();
    status("Setting all to "+ hex(c,6));
    int r = int(red(c));
    int g = int(green(c));
    int b = int(blue(c));
    try { 
      linkm.ctrlmSetSendAddress( ctrlmaddr, 0,0 );
      linkm.setFadeSpeed(ctrlmaddr, 2);
      linkm.fadeToRGB( ctrlmaddr, r,g,b);
    } catch(IOException ioe) {
      status("ioerror: "+ioe.getMessage());
    }
  }
  else if( name.equals("Set FreeMs") ) {
    ControlWindow w = controlP5.window("SetFreeMs");
    if( w == null ) {
      makeFreeMsWindow();
    }
    controlP5.window("SetFreeMs").show();
  }
  // FreeM window
  else if( name.matches("[A-Z][A-Z]") ) {
    boolean shouldSet = freemSet.getState();
    char c1 = name.charAt(0);
    char c2 = name.charAt(1);
    int freemaddr = 1 + (c1 - 'A') * 10 + (c2 - 'A'); // make linear 1-100
    status("Selecting FreeM "+c1+c2+ " ("+freemaddr+")...");
    if( !connectIfNeeded() ) {
      status("no LinkM detected");
      return;
    }
    try { 
      if( shouldSet ) {
        status("set address for "+c1+c2);
        linkm.ctrlmWriteFreeMAddress( ctrlmaddr, freemaddr );
      } else {
        linkm.ctrlmSetSendAddress( ctrlmaddr, freemaddr, 0 );
        //linkm.setFadeSpeed(0,20);  // FIXME: why doesn't this work?
        linkm.playScript(0, 2, 2, 0); // white, once
        linkm.ctrlmSetSendAddress( ctrlmaddr, 0, 0 ); // everyone again
        //println("viewing!");
      }
    } catch(IOException ioe) { 
      status("ioerror "+ioe.getMessage());
    }
  }
  else {
    // else image button selected
    if( id >= 100 ) {
      imgn =  id - 100 ;
      displayColors( dispToggle.getState() );
    }
  }
  
}

// --------------------------------------------------------------------------

void setupImages() {
  for(int i=0; i<btncnt; i++) {
    setupImage(imgf[i],i);  // FIXME: duh
  }
}

void setupImage(String imgpath, int n) {

  String f = new String(imgpath);
  PImage p = loadImage(f);
  if( p == null ) { 
    f = sketchPath + File.separator + imgpath;
    p = loadImage(f);
  }
  imgf[n] = f;
  imga[n] = p;
  try { 
    imgb[n] = (PImage)p.clone();
  } catch(CloneNotSupportedException e){ }

  //imgf[n] = new String(imgpath);
  //imga[n] = loadImage(imgf[n]);  // Load the image into the program  
  //imgb[n] = loadImage(imgf[n]);
  if( imga[n] == null ) {
    try { 
      imga[n] = (PImage)(jellyaimg.clone());
      imgb[n] = (PImage)(jellybimg.clone());
    } catch(CloneNotSupportedException e) { }
  }
  if( imgb[n].width  != 10 &&
      imgb[n].height != 10 ) {
    imga[n].resize(dispw,disph);
    imgb[n].resize(gridw,gridh);
  } 

  try { 
    PImage img = (PImage)(imga[n].clone());
    img.resize(40,40);
    btni[n].setImage( img );
  } catch(Exception e){ }
    
}

//
void chooseImage(int n) {
  
  if( n < 0 || n> btncnt) {
    n = 0;
    imgpath = "jelly.jpg";
  } 

  String lastimgpath = imgpath;
  
  imgpath = selectInput("choose image...");
  status("imgpath: "+imgpath);
  
  imgpath = (imgpath == null) ? lastimgpath : imgpath;

  setupImage(imgpath, n);

  imgn = n;

  dispMode = 0;

}

// save a set of images
public void saveSet() {
  String f = setname;
  // remove absolute path to images if in same dir as app
  for( int i=0; i<imgf.length; i++) {
    imgf[i] = imgf[i].replaceAll(sketchPath+File.separator,"");
  }
  saveStrings( f, imgf);
}

// load a set of images
// FIXME: this logic is too convoluted
public boolean loadSet(boolean ask) {
  if( !ask ) {
    status("loading set from '"+setname+"'");
    imgf = loadStrings( setname );
    if( imgf != null ) { // found file in current directory
      setupImages();
      return true;
    }
    // else bad file, so ask for new one
  }
  
  String str = "Please find 'gridset.txt' file"; 
  String f = selectInput(str);
  if( f != null ) {     // user did select file 
    setname = f;
    imgf = loadStrings( setname );
    if( imgf != null ) { // yay, we got strings
      setupImages();
      return true;
    } 
    // else bad file, so create below
  }
  else { // user did not select file
    return false;
  }
  
  // else empty file or user cancelled
  imgf = new String[btncnt];
  for( int i=0; i<btncnt; i++ ) {
    imgf[i] = "jelly.jpg";
  }
  
  setupImages();
  return true;
}

// ----------------------------------------------------------------------------

// trigger a display of colors
public void displayColors(boolean shouldSend) {
  status("Displaying "+imgn+"...");
  if( !shouldSend ) return;
  if( !connectIfNeeded() ) {
    status("no LinkM detected");
    return;
  }
  try { 
    //linkm.setFadeSpeed(ctrlmaddr, 1);
    //linkm.pause(100);
    linkm.ctrlmPlayFreeMColorSpot(ctrlmaddr, imgn, 'c'); // FIXME:
    //linkm.ctrlmPlayFreeMColorSpot(ctrlmaddr, imgn, 'n'); // FIXME:
  } catch( IOException ioe ) {
    status("ioerror: "+ioe.getMessage());
  }

}

// Upload a display of colors 
public void uploadColors() {
  status("Uploading image "+imgn+"...");
  
  if( !connectIfNeeded() ) {
    status("no LinkM detected");
    return;
  }
  
  try { 
    uploading = true;
    int n = imgn; // save in case user changes displayed image
    int[] px = imgb[n].pixels;
    for( int i=0; i< px.length; i++ ){
      status("sending pixel "+i);
      int pixel = px[i];
      int r = int(red(pixel));
      int g = int(green(pixel));
      int b = int(blue(pixel));
      
      int freem_addr = i + 1; // can't use zero == broadcast

      linkm.ctrlmSetSendAddress( ctrlmaddr, freem_addr, 0);  // quick, in ctrlm
      linkm.ctrlmSetFreeMColorSpot( ctrlmaddr, n,  r,g,b );
      //linkm.ctrlmSetFreeMColorSpot( ctrlmaddr, i,  r,g,b ); // wrong,but debug
      delay( 100 );
      if( uploading == false ) { break; }
    }

    linkm.ctrlmSetSendAddress( ctrlmaddr, 0,0 ); // talk to everyone again
    
  } catch( IOException ioe ) {
    status("io error:"+ioe.getMessage());
  }
  uploading = false;
  status("uploading done");

}	

// simple stupid helper class to run uploader in another thread
class Uploader implements Runnable {
  public void run() {
    uploadColors();
  } 
}

// ------------------------------------------------------------------
// open up the LinkM and set it up if it hasn't been
boolean connectIfNeeded() {
  if( !isConnected ) {
    status("connecting");
    try { 
      linkm.open();
      linkm.i2cEnable(true);
      linkm.pause(1000); // wait for bus to stabilize // bigger delay for CtrlM
      byte[] addrs = linkm.i2cScan(1,100);  // FIXME: not a full scan
      int cnt = addrs.length;
      status("found "+cnt+" blinkm-like devices");
      if( cnt>0 ) {
        byte[] blinkmver = linkm.getVersion( addrs[0] );
        if( blinkmver[0] == 'b' ) {
          status("found ctrlm");
          if( blinkmver[1] == 'b' ) { 
            status("found ctrlm (new)");
            ctrlmaddr = addrs[0]; 
          }
          else {
            status("found ctrlm (old)");
          }
        } else { 
          status("found non-ctrlm device");
        }
        ctrlmaddr = addrs[0];    
      }
      else {
        status("linkm found, but no ctrlm");
      }
    } catch(IOException ioe) {
      status("no linkm found");
      return false;
    }
    linkm.pause(200);  // FIXME: do we need this? 
    isConnected = true;
  }
  return true; // connect successful
}






