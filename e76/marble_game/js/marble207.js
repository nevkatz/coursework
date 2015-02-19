// JavaScript Document

var reCal = false;
var started = false;
// establishes canvas and context
var canvas = document.getElementById('myCanvas');
var canvasMarble = document.getElementById('myCanvasMarble');

var portraitWidth;
var recalX = 0;
var recalY = 0;

// contexts
var context = canvas.getContext('2d');
var contextMarble = canvasMarble.getContext('2d');

// is game paused?
var paused = true;

// dimensions of square tile

var tileDim;

// score
var myScore = 0;

// timer that persists across levels
var gameSeconds = 0;

// timer that is reset with each level
var roundSeconds = 0;

// objects
var marble1;
var geo1;
var myGoal;

// minFlames - persists across levels
var minFlames = 1;
var myLevel = 1;

var s;

// damage timer
var myWalls = [];
var myCoins = [];
var myFlames = [];

// stores maps - 2D arrays of 0's and 1's - do show where these objects are. 
//var wallMap = [];
//var coinMap = [];
//var flameMap = [];

var lowerBounds;
var rightBounds;
var upperBounds;
var leftBounds;

var counter = 0;



/* websocket */
var socket = 'ws://echo.websocket.org';

var connection;

function webSocket()
{

    try
    {
        connection = new WebSocket(socket);

        

        connection.onopen = function ()
        {
            connection.send('Ping');
        }

        connection.onerror = function (error)
        {
             console.log('websocket error ' + error);
        }

        connection.onmessage = function (e)
        {
             console.log('server: ' + e.data);
        }
    }
    catch (err)
    {
        console.log('no websocket');
    }
}

init();

function detectStart()
{
       if (Math.abs(window.orientation) === 90 && started == false)
        {
			switchToGame();
            setCanvas();
            started = true;
			window.onorientationchange = readDeviceOrientation;
        }
    
}
function readDeviceOrientation()
{
    var pauseMsg = document.getElementById('gamePaused');
	
   
        if (Math.abs(window.orientation) !== 90)
        {
            pause(pauseMsg);
        }
        else
        {
            unPause(pauseMsg);
        }
    
   
}

function init()
{
    // sets up web socket

    webSocket();

    if (Math.abs(window.orientation) === 90)
    {
	
		portraitWidth = window.innerHeight;
        setCanvas();
        started = true;
		window.onorientationchange = readDeviceOrientation;
    }
	else
	{
		portraitWidth = window.innerWidth;
		rotateReminder();
		
   		window.onorientationchange = detectStart;
     
	}
   
}

/*
 *  Switches to the message screen on rotate
 */
function switchToMessage(moveIt)
{
	var gamescreen = document.getElementById('game_screen');
    var menuscreen = document.getElementById('menu_screen');

	var message = document.getElementById('message');
	
	if (moveIt == true) 
	{
	
		switch(window.innerWidth)
		{
			case 1024:
			case 768: 
			message.style.left = '12%';
			break;
			
			break;
			default: 
			message.style.left = '20%';
			break;
		}
		
	}
	
	
   
	
	 if (gamescreen) 
	 {
		 menuscreen.style.width = '150%';
		 gamescreen.style.display = 'none';
		
	 }
	
    if (menuscreen) menuscreen.style.visibility = 'inherit';
}

 /*
  *   Switches to game mode on rotate
  */
function switchToGame()
{
	 var gamescreen = document.getElementById('game_screen');
    var menuscreen = document.getElementById('menu_screen');
	var pauseMsg = document.getElementById('gamePaused');
	

    if (gamescreen)
	{
		gamescreen.style.display = 'block';
		pauseMsg.style.visibility = 'collapse';
		paused = false;
	  
	}

    if (menuscreen) menuscreen.style.visibility = 'collapse';
}

/*
 * switches to message screen, prints message if device is in portrait mode at game's start
 */
function rotateReminder()
{
	switchToMessage(false);
	
	message('To start, rotate your device to Landscape Mode.');
	
}
/*
 *  Used for reminding the user to rotate the device to landscape mode - either during pauses or at start of game. 
 */
function message(myMessage)
{
	var message = document.getElementById('message');

	var menuscreen = document.getElementById('menu_screen');
	message.innerHTML = myMessage;
	
	menuscreen.style.width = '150%';
}

/*
 *  Sets canvas dimensions
 */
 
function setCanvas()
{
    /* canvas & tiles */

    var canvas1 = document.getElementById('myCanvas');
    var canvas2 = document.getElementById('myCanvasMarble');

    var cWidth;
    var cHeight;

    if (window.innerWidth == 480 || window.innerWidth == 320)
    {
        tileDim = 20;

        cWidth = '480px';
        cHeight = '240px';

    }
    else if (window.innerWidth == 1024 || window.innerWidth == 768)
    {
        tileDim = 40;

        cWidth = '1024px';
        cHeight = '622px';

    }
    else
    {
        if (window.innerWidth < 768)
        {
            tileDim = 20;



            try
            {
                cHeight = window.innerHeight - 57;
            }
            catch (err)
            {
                cHeight = '260px';
            }


        }
        else if (window.innerWidth >= 768)
        {
            tileDim = 40;

            try
            {
                cHeight = window.innerHeight - 80;
            }
            catch (err)
            {
                cHeight = '528px';
            }

        }
        adjustNav();
    }
    // console.log('innerHeight: ' + window.innerHeight);
    cWidth = window.innerWidth;

    canvas1.setAttribute('width', cWidth);
    canvas1.setAttribute('height', cHeight);

    canvas2.setAttribute('width', cWidth);
    canvas2.setAttribute('height', cHeight);






    geo1 = new geo(0, 0);

    // if no click, start game in the time frame below

    /*******TURN ON WHEN DONE WITH GAME OVER***/
    var myTimer = window.setTimeout(function ()
    {

        gameStart();


    }, 100);

    // if click, clear timer and start game
  

    navListeners();

    instantiateTimer();
}

function adjustNav()
{
    var pos;
    try
    {

        pos = window.innerHeight - 37;

        var nav = document.getElementById('nav');

        var stats = document.getElementById('stats');

        stats.style.width = window.innerWidth + 'px';

        nav.style.width = window.innerWidth + 'px';

        nav.style.top = pos + 'px';
    }
    catch (err)
    {
        nav.style.top = canvas.height + 20;
        // console.log('error');
    }
}

/*
 *  Starts the timer
 */
function instantiateTimer()
{
    var gameTimer = setInterval(function ()
    {

        if (!paused)
        {
            gameSeconds++;
           	 updateTime();
        }

    }, 1000);
}

function navListeners()
{
    var pauseBtn = document.getElementById('pauseButton');
    var pauseMsg = document.getElementById('gamePaused');
    var quitGame = document.getElementById('quit');

    var hiScores = document.getElementById('scores');

    var cal = document.getElementById('calibrate');


    // takes you back to menu
    mListener(quitGame)

    sListener(hiScores);


    cal.addEventListener('touchstart', function ()
    {

        reCal = true;
        //openCalibration();



    })

    pauseBtn.addEventListener('touchstart', function ()
    {
		var pauseMsg = document.getElementById('gamePaused');
        if (paused) unPause(pauseMsg);
        else pause(pauseMsg);

    });
}

/*
 *  Pauses game - if in landscape mode, game stays visible. 
 */
function pause(pauseMsg)
{
    paused = true;
	
	if (Math.abs(window.orientation)===90)
	{
	
    pauseMsg.style.visibility = 'inherit';
    pauseMsg.innerHTML = 'Game Paused.';
	
	}
	else
	{
	switchToMessage(true);
	
	message('Game Paused. Rotate to resume.');
	}
}

function unPause(pauseMsg)
{
    paused = false;
	
	message('');
	switchToGame();
    pauseMsg.style.visibility = 'collapse';

    reCal = true;
   
}

function openCalibration()
{
    reCal = true;

    //setTimeout(function () { reCal = false; }, 500);
}

/* 
 * Starts a new game
 */
function newGame()
{
    var gameOver = document.getElementById('game_over');

    gameOver.style.visibility = 'collapse';

    resetStats();
    gameStart();
}

/*
 *  Resets variables when a new level starts
 */

function resetStats()
{
    recalX = 0;
    recalY = 0;
    reCal = false;

    gameSeconds = 0;
    myScore = 0;
    myLevel = 1;
}

function nListener(nBtn)
{
    nBtn.addEventListener('touchstart', function ()
    {

        newGame();

    });

    nBtn.addEventListener('mousedown', function ()
    {

        newGame();

    });

  

}

/*
 *  Quit game listener
 */
function qListener(qBtn)
{


    qBtn.addEventListener('touchstart', function (event)
    {
        storeRecord('http://birdleymedia.com/');

    });

    qBtn.addEventListener('mousedown', function (event)
    {
        storeRecord('http://birdleymedia.com/');

    });
}

/*
 *  Main menu button listener
 */
function mListener(mBtn)
{

    mBtn.addEventListener('touchstart', function (event)
    {
        storeRecord('http://birdleymedia.com/e76/main.html');

    });
}

function sListener(sBtn)
{
    sBtn.addEventListener('touchstart', function (event)
    {
        window.location.href = 'http://birdleymedia.com/e76/scores.html';

    });

    
}

/*
 *  Chooses bg color at random, at the start of a level
 */
function bgColor()
{
    var colors = ['#283c6b', '#7696d8', '#fcc', '#c6f2e7', '#c298f2', '#f7c8f6', '#d7ddea', '#68ea44']

    var index = Math.floor(Math.random() * colors.length);

    canvas.style.backgroundColor = colors[index];
}
/*
 *  converts degrees to radians for calibration 
 */

function degRad(deg)
{
    var rad = ((deg * Math.PI) / 180)

    return rad;

}

/*
 *  gets orientation data and sets variables that are used to move the marble
 */
function getOrientation()
{
    window.addEventListener('deviceorientation', function (orientData)
    {
        if (marble1)
        {
            var obj = [];

            obj.x = orientData.gamma;
            obj.y = -1 * orientData.beta;
            obj.z = orientData.alpha;

            if (reCal == true)
            {
                contextMarble.clearRect(0, 0, canvas.width, canvas.height);
                recalX = -1 * obj.x;
                recalY = -1 * obj.y;
                reCal = false;
            }

            var spd = 8;

            var velX = Math.sin(degRad(obj.y + recalY))

            var velY = Math.sin(degRad(obj.x + recalX))

            marble1.myX = spd * velX * marble1.speed;
            marble1.myY = spd * velY * marble1.speed;
        }


      



    });
}

/*
 *  Starts the game
 */
function gameStart()
{
    var s = new CanvasState(document.getElementById('canvas'));

    getOrientation();

    var gamescreen = document.getElementById('game_screen');
    var menuscreen = document.getElementById('menu_screen');
	var levelcomplete = document.getElementById('level_complete');

    var timeStat = document.getElementById('time');

    if (timeStat) timeStat.innerHTML = 'Time: 0:00';

    gamescreen.style.visibility = 'inherit';

    if (menuscreen) menuscreen.style.visibility = 'collapse';
	if (levelcomplete) levelcomplete.style.visibility = 'collapse';

    setBoard();
    bgColor();
    updateLevel();
    scoreButton();
    paused = false;


}

/*
 *   Only show score button if there are scores. 
 */
function scoreButton()
{
    var score = document.getElementById('scores');
    if (scanRecord())
    {
     
        score.style.visibility = 'inherit';
    }
    else
    {
       
        score.style.visibility = 'collapse';
    }

}
/*
 *     Clear Functions
 */
function clearBoard()
{
	
	
    /* clear arrays of objects */
    clearGroup(myWalls);

    clearGroup(myFlames);

    clearGroup(myCoins);
	
	/* map arrays are local variables that ceased to be used after walls, flames, and coins are instantiated.*/

    var timer = window.setInterval(function ()
    {
        clearMarble(timer)
    }, 10);
}

/*
 *  Nullifies objects after a game ends
 */
function clearMarble(timer)
{
    if (myWalls.length == 0 && myCoins.length == 0 && myFlames.length == 0)
    {
        marble1 = null;

        contextMarble.clearRect(0, 0, canvas.width, canvas.height);

        timer = window.clearInterval(timer);

    }
}

function clearGroup(group)
{
    while (group.length > 0)
    {
        group[0] = null;
        group.splice(0, 1);
    }
}

function clearMap(map)
{
    for (var i = 0; i < canvas.width/tileDim; i++)
    {
        while (map[i].length > 0)
        map[i].splice(0, 1);
    }
}


/*
 *  Instantiate canvas 
 */

// set up canvas state, which stores some global info
function CanvasState(canvas)
{
    this.valid = false;
    this.shapes = [];

    var myState = this;

    this.interval = 35;

    var myInterval = setInterval(function ()
    {

        if (!paused) moveMarble(myInterval);

        myState.draw(myInterval);

    }, myState.interval);



}








/*
 *   **** NPC Classes - wall, coin, flame *****
 */

/*
 *  Wall code
 */

// wall
function wall(posX, posY, width, height)
{
    this.posX = posX;
    this.posY = posY;

    this.width = width;
    this.height = height;

    this.drawn = false;

    this.name = 'wall';
}

// sets up the function for the wall class. 
wall.prototype.draw = function ()
{
    if (!this.drawn)
    {
        var x1 = this.posX - this.width / 2;
        var y1 = this.posY - this.height / 2;

        try
        {
            context.fillStyle = wallGradient(x1, y1);
        }
        catch (err)
        {
            context.fillStyle = '#f7d38a';
        }


        context.strokeStyle = '#7f5016';
        context.lineWidth = 1;
        context.fillRect(x1, y1, this.width, this.height);

        context.strokeRect(x1, y1, this.width, this.height);

        this.drawn = true;
    }

}

function wallGradient(x1, y1)
{
    var xStart = x1 - tileDim * 0.2;

    var yStart = y1 - tileDim * 0.2;

    var xExtent = x1 + tileDim * 0.7;

    var yExtent = y1 + tileDim * 0.7;


    var grd = context.createLinearGradient(xStart, yStart, xExtent, yExtent);

    grd.addColorStop(0, '#f7d38a');

    grd.addColorStop(1, '#b57d0e');

    return grd;

}




/*
 *  coin class
 */

function coin(posX, posY, radius, value, color, stroke)
{
    this.posX = posX;
    this.posY = posY;

    this.radius = radius;

    this.value = value;

    this.taken = false;

    this.drawn = false;

    this.color = color;

    this.stroke = stroke;
}

/*
 *  renders basic coin
 */
coin.prototype.draw = function ()
{
    var x1 = this.posX;
    var y1 = this.posY;

    if (!this.drawn)
    {
        // diamond shape
        context.beginPath();

        //vertical line, right
        context.moveTo(x1 + this.radius / 1.5, y1 + this.radius / 2);
        context.moveTo(x1 + this.radius / 1.5, y1 - this.radius / 2);

        //upper middle point
        context.lineTo(x1, y1 - this.radius);

        // vertical line, left
        context.lineTo(x1 - this.radius / 1.5, y1 - this.radius / 2);
        context.lineTo(x1 - this.radius / 1.5, y1 + this.radius / 2);

        // lower middle point
        context.lineTo(x1, y1 + this.radius);

        // back to start
        context.lineTo(x1 + this.radius / 1.5, y1 + this.radius / 2);
        context.closePath();
        
        context.fillStyle = this.color;
        context.fill();
        context.lineWidth = 1;
        context.strokeStyle = this.stroke;
        context.stroke();

        this.drawn = true;
    }
}

/*
 *  flame class
 */

function flame(posX, posY, radius, i)
{
    this.posX = posX;
    this.posY = posY;

    this.radius = radius;

    this.drawn = false;

    this.name = 'flame' + i;
}


/*
 * renders a flame
 */
flame.prototype.draw = function ()
{
    counter++;
    var dir = 0;

    var prevSec = parseInt(counter - 1 / 3000)
    var sec = parseInt(counter / 3000);

    var prob = Math.floor(Math.random() * myFlames.length);

    if (!this.drawn)
    {
        

        var x1 = this.posX;
        var y1 = this.posY;


        

        context.beginPath();

        // top left corner
        context.moveTo(x1 - this.radius + dir, y1 - this.radius);

        // go down
        context.lineTo(x1 - this.radius, y1);

        // curve
        context.quadraticCurveTo(x1, y1 + this.radius * 2, x1 + this.radius, y1);

        // to the top right corner
        context.lineTo(x1 + this.radius + dir, y1 - this.radius);

        // first down
        context.lineTo(x1 + this.radius / 2, y1 - this.radius / 2);

        // first up
        context.lineTo(x1 + dir, y1 - this.radius);

        //second down
        context.lineTo(x1 - this.radius / 2, y1 - this.radius / 2);

        //second up
        context.lineTo(x1 - this.radius + dir, y1 - this.radius);
        context.closePath();

        //context.restore();

        context.fillStyle = 'red';
        context.fill();
        context.lineWidth = 3;
        context.strokeStyle = 'orange';
        context.stroke();

        this.drawn = true;

    }
}


/*
 *  Goal class
 */
function goal(posX, posY, radius, color, stroke)
{
    this.posX = posX;
    this.posY = posY;

    this.drawn = false;

    this.radius = radius;

    this.stroke = stroke;

    this.color = color;

}

/*
 *  Drawing a goal - looks like a ninja star
 */
goal.prototype.draw = function ()
{
    if (!this.drawn && !paused)
    {
        // console.log("drawing goal at " + this.posX + "," + this.posY);
        var x1 = this.posX;
        var y1 = this.posY;

        // diamond shape
        context.beginPath();

        //right side
        context.moveTo(x1 + this.radius / 4, y1 + this.radius / 4);

        //point 

        context.lineTo(x1 + this.radius, y1);


        context.lineTo(x1 + this.radius / 4, y1 - this.radius / 4);

        //upper middle point
        context.lineTo(x1, y1 - this.radius);

        // left side
        context.lineTo(x1 - this.radius / 4, y1 - this.radius / 4);

        // point
        context.lineTo(x1 - this.radius, y1);

        // end left side
        context.lineTo(x1 - this.radius / 4, y1 + this.radius / 4);

        // lower middle point
        context.lineTo(x1, y1 + this.radius);

        // back to start
        //context.lineTo(x1+this.radius/1.5, y1+this.radius/2);
        context.closePath();
        //context.arc(x1, y1, this.radius, 0, 2*Math.PI, false);
        context.fillStyle = this.color;
        context.fill();
        context.lineWidth = 1;
        context.strokeStyle = this.stroke;
        context.stroke();

        this.drawn = true;
    }
}




/*
 *  Used for debugging from a computer
 */

window.addEventListener('keydown', function (e)
{
    if (marble1)
    {
        switch (e.keyCode)
        {
            case 37:
                if (!marble1.hitLeft) marble1.posX -= 5;
                break;

            case 38:
                if (!marble1.hitUp) marble1.posY -= 5;
                break;

            case 39:
                if (!marble1.hitRight) marble1.posX += 5;
                break;

            case 40:
                if (!marble1.hitDown) marble1.posY += 5;
                break;
        }
    }
});

/*
 *  sets the boundaries of the screen
 */
function setBounds()
{
    lowerBounds = canvas.height - marble1.radius;
    rightBounds = canvas.width - marble1.radius;
    upperBounds = marble1.radius;
    leftBounds = marble1.radius;
}


/*
 *  Generates random placement for goal
 */
function randomVert()
{
    var vert = tileDim * 1.5 + (Math.floor(Math.random() * 6)) * tileDim;

    return vert;
}

function randomHoriz()
{
    var horiz = tileDim / 2 + (Math.floor(Math.random() * 3)) * tileDim;

    return horiz;
}
/*
 *  Sets up marble, goal, and walls on board.  addWalls will call the addCoins function; addCoins calls addFlames.
 */

function setBoard()
{
    var marbleVert = randomVert();

    var marbleHoriz = randomHoriz();

    marble1 = new Marble(marbleHoriz, marbleVert);

    var goalVert = randomVert();

    var goalHoriz = ((Math.floor(canvas.width / tileDim) - 3) + 0.5) * tileDim
    //	var goalHoriz = tileDim*20.5;

    myGoal = new goal(goalHoriz, goalVert, tileDim, 'white', 'orange');

    setBounds();

    updateHealth();
    updateScore();


     addWalls();
 
}

/*
 *  Used to computer the probability of an object appearing on a certain tile. 
 */
function findChance(lowestProb, multiplier)
{
    var chance = lowestProb - myLevel * multiplier;

    chance = (chance < 2) ? 2 : chance;

    return chance;
}

/*
 *   Adds walls
 */
function addWalls()
{

	var wallMap = [];
    var h = Math.floor(canvas.height / tileDim)

    var w = Math.floor(canvas.width / tileDim)

    var blockX = 0;
    var blockY = 0;

    var vertBlocks = [];

    // vert blocks on each x-coordinate start out as zero. 
    for (var k = 0; k < w; k++) vertBlocks[k] = 0;

    var spaceX = 0;
    var spaceY = 0;

    for (var i = 0; i < h; i++) // i = vert
    {
        wallMap[i] = [];

        for (var j = 0; j < w; j++) // j = horiz
        {
            var tileAbove;
            var tileLeft;
            var tileLeft2;
            var tileAbove2;
            var tileUpLeft;
            var tileUpRight;

            if (i == 0 || j == 0)
            {
                tileAbove = 0;
                tileLeft = 0;
                tileUpLeft = 0;
                tileUpRight = 0;
            }
            else
            {
                tileAbove = wallMap[i - 1][j]

                tileLeft = wallMap[i][j - 1];

                tileUpLeft = wallMap[i - 1][j - 1];

                tileUpRight = wallMap[i - 1][j + 1];
            }
            // check tiles two behind
            if (j <= 1)
			 tileLeft2 = 0;
            else 
			tileLeft2 = wallMap[i][j - 2];

            if 
			(i <= 1) 
			tileAbove2 = 0;
            else 
			tileAbove2 = wallMap[i - 2][j];

            var chance = findChance(5, 1);

            var wallChance = Math.ceil(Math.random()*chance);

            // set tile coordinates
            var myX = (j + 0.5) * tileDim;

            var myY = (i + 0.5) * tileDim;

            // if chance favors a wall, we are not near the player start, 
            // and we have not exceeded a wall length of 4
            if ((wallChance == 1

            && !inVicinity(myX, myY, marble1, tileDim) // not near the starting point

            &&
            !inVicinity(myX, myY, myGoal, tileDim)

            && blockX < 4 // rows are no longer than 4

            &&
            (spaceX > 2 || spaceX == 0)



            &&
            tileAbove == 0

            && vertBlocks[j] < 2 // the space directly above cannot have a block

            &&
            tileUpLeft == 0 // no diagonals allowed

            &&
            tileUpRight == 0) // no diagonals allowed

            ||

            // if we started a block up top, we want to continue it, and there is no block on the left
            (vertBlocks[j] < 3 && wallChance == 1 && tileAbove == 1 && tileLeft == 0 // there cannot be a block directly on the left
            &&
            !inVicinity(myX, myY, marble1, tileDim) && !inVicinity(myX, myY, myGoal, tileDim))

            ||

            // if we have the beginnings of a t-formation
            tileAbove == 1 && wallChance == 1 && tileUpLeft == 1 && tileUpRight == 1 && vertBlocks[j] < 2 && !inVicinity(myX, myY, myGoal, tileDim) && tileLeft == 0) // limit height of blocks
            {
                spaceY = 0;
                spaceX = 0;


                buildWall(myX, myY, tileDim);
                wallMap[i].push(1);

                blockX++;

                vertBlocks[j]++;

                
            }
            else
            {
                blockX = 0;
                blockY = 0;

                // start counting vertical blocks over again.
                vertBlocks[j] = 0;

                wallMap[i].push(0);

                spaceX++;
                spaceY++;
            }


        }
      
    }
	 addCoins(wallMap);
}
// give marble breathing room
function inVicinity(myX, myY, Obj, dim)
{
    var here = false;
    for (var i = 0; i < 3; i++)
    {
        for (var j = 0; j < 3; j++)
        {
            var xTile = (i - 1) * dim;
            var yTile = (j - 1) * dim;

            var spaceX = Obj.posX + xTile;
            var spaceY = Obj.posY + yTile;



            if (myX == spaceX && myY == spaceY)
            {
                // // // console.log('compare ' + myX+','+myY+' with '+spaceX +','+spaceY);
                //	// // // console.log("here");
                here = true;
            }
        }
    }

    return here;

}

function atLoc(myX, myY, Obj)
{
    if (myX == Obj.posX && myY == Obj.posY) return true;
    else return false;
}

// builds one wall tile

function buildWall(myX, myY, dim)
{


    var myWall = new wall(myX, myY, dim, dim)

    myWall.draw();

    myWalls.push(myWall);

}
/*
 *  Adds coins
 */
function addCoins(wallMap)
{
	var coinMap = [];

    var h = Math.floor(canvas.height / tileDim)

    var w = Math.floor(canvas.width / tileDim)

    for (var i = 0; i < h; i++) // i = vert
    {

        coinMap[i] = [];
        for (var j = 0; j < w; j++) // j = horiz
        {

            /*** neighboring tiles**/
            var tileAbove;
            var tileLeft;
            var tileUpLeft;
            var tileUpRight;

            if (i == 0 || j == 0)
            {
                tileAbove = 0;
                tileLeft = 0;
                tileUpLeft = 0;
                tileUpRight = 0;
            }
            else
            {
                tileAbove = coinMap[i - 1][j]

                tileLeft = coinMap[i][j - 1];

                tileUpLeft = coinMap[i - 1][j - 1];

                tileUpRight = coinMap[i - 1][j + 1];
            }

            /*********/


            // set coin chance to zero
            var coinChance = 0;

            // translate loops to coordinates
            var myX = (j + 0.5) * tileDim;

            var myY = (i + 0.5) * tileDim;

            //// // // console.log("coin coord: " + myX + "," + myY);
            if (wallMap[i][j] == 0 
			&& tileAbove == 0 
			&& tileUpLeft == 0 
			&& tileUpRight == 0
			 && tileLeft == 0
			  && !inVicinity(myX, myY, marble1, tileDim)
			   && !inVicinity(myX, myY, myGoal, tileDim))
            {
                var chance = findChance(15, 1);

                coinChance = Math.ceil(Math.random() * chance);
            }



            if (coinChance == 1)
            {
                buildCoin(myX, myY);
                coinMap[i].push(1)
            }
            else
            {
                coinMap[i].push(0);
            }
        }
    }
	addFlames(wallMap, coinMap);
}



/*
 *  Set up game screen by adding walls, coins, flames
 */

// set up coins
function buildCoin(myX, myY)
{
    // console.log('buildCoin');
    var color;
    var stroke;
    // make value determined by color
    var value = (Math.ceil(Math.random() * 2)) * 10;
    //// // // console.log("value: " + value);

    var radius = tileDim / 2;


    try
    {
        switch (value)
        {
            case 10:
                // console.log('10');
                stroke = 'orange';
                color = coinGradient(0, myX, myY);
                // console.log('color: ' + color);
                break;

            case 20:
                // console.log('20');
                stroke = '#448241';
                color = coinGradient(1, myX, myY);
                break;
        }
    }
    catch (err)
    {
        switch (value)
        {
            case 10:
                stroke = 'orange';
                color = 'yellow';
                break;

            case 20:
                stroke = '#448241';
                color = 'green';
                break;
        }
    }

    var myCoin = new coin(myX, myY, radius, value, color, stroke);

    myCoin.draw();



    myCoins.push(myCoin);
  
}

/*
 *   Adds flames
 */
function addFlames(wallMap, coinMap)
{
	var flameMap = [];
    var flameCounter = 0;

    var mostRecentFlame = 0;

    var roundCounter = 0;


    var h = Math.floor(canvas.height / tileDim)

    var w = Math.floor(canvas.width / tileDim)

    for (var i = 0; i < h; i++) // i = vert
    {

        flameMap[i] = [];
        for (var j = 0; j < w; j++) // j = horiz
        {

            /*** neighboring tiles**/
            var tileAbove;
            var tileLeft;
            var tileUpLeft;
            var tileUpRight;

            var flameAbove;
            var flameUpLeft;
            var flameUpRight;
            var flameLeft;

            if (i == 0 || j == 0)
            {
                tileAbove = 0;
                tileLeft = 0;
                tileUpLeft = 0;
                tileUpRight = 0;
            }
            else
            {
                tileAbove = wallMap[i - 1][j]

                tileLeft = wallMap[i][j - 1];

                tileUpLeft = wallMap[i - 1][j - 1];

                tileUpRight = wallMap[i - 1][j + 1];

                flameAbove = flameMap[i - 1][j]

                flameLeft = flameMap[i][j - 1];

                flameUpLeft = flameMap[i - 1][j - 1];

                flameUpRight = flameMap[i - 1][j + 1];
            }

            /*********/


            // set coin chance to zero
            var flameChance = 0;

            // translate loops to coordinates
            var myX = (j + 0.5) * tileDim;

            var myY = (i + 0.5) * tileDim;

            //// // // console.log("coin coord: " + myX + "," + myY);
            if (wallMap[i][j] == 0 
			&& coinMap[i][j] == 0 
			&& !inVicinity(myX, myY, marble1, tileDim) 
			&& !inVicinity(myX, myY, myGoal, tileDim) 
			&& tileAbove == 0 && tileUpLeft == 0
			 && tileUpRight == 0 
			 && tileLeft == 0

            && flameUpLeft == 0 
			&& flameUpRight == 0 
			&& flameAbove == 0)

            {
                var coldSpellLength = roundCounter - mostRecentFlame;
                if (flameCounter < minFlames && coldSpellLength > 20)
                // if we don't have min. number of flames 
                //  and if we haven't seen a flame in 20 rounds
                {
                    flameChance = 1;
                }
                else
                {
                    var chance = findChance(100, 1);

                    flameChance = Math.ceil(Math.random() * chance);
                }
            }



            if (flameChance == 1)
            {
                buildFlame(myX, myY);
                flameMap[i].push(1)
                flameCounter++;
                mostRecentFlame = roundCounter;
            }
            else
            {
                flameMap[i].push(0);
            }

            roundCounter++;
        }
    }
    minFlames = flameCounter;
}

// set up flames

function buildFlame(myX, myY)
{

    // flame has to be pretty small so canvas can redraw itself without affecting walls. 
    var radius = tileDim * 0.3;

    var myFlame = new flame(myX, myY, radius);

    myFlame.draw();

    myFlames.push(myFlame);
}


/*
 *  Draw functions for CanvasState
 */

CanvasState.prototype.draw = function (myInterval)
{
    getGeo();

    if (!marble1)
    {
        myInterval = window.clearInterval(myInterval);
        return 0;
    }
    if (!paused)
    {



        // clear part of canvas surrounding marble
        eraseMe(marble1, 3, contextMarble);

        centerHit();

        coinTest();

        checkGoal();


        for (var i = 0; i < myWalls.length; i++)
        {
            var myObject = myWalls[i];

            myObject.draw();
        }

        for (var i = 0; i < myCoins.length; i++)
        {
            var myObject = myCoins[i];

            // if coin hasn't been taken, draw it!
            if (!myObject.taken) myObject.draw();

            else
            {
                myObject.posY = -2000;
                myObject.posX = -2000;
            }
        }

        for (var i = 0; i < myFlames.length; i++)
        {
            var myObject = myFlames[i];

            myObject.draw();

        }
        var color = checkDamage();

        myGoal.draw();

        marble1.draw(color);

    }
}

/*
 *  If marble is in a flame it turns red. 
 */
function checkDamage()
{
    var dmg = flameTest();

    var grd;

    if (dmg)
    {
        try
        {
            grd = marbleGradient(1);
        }
        catch (err)
        {
            grd = 'red';
        }

    }
    else
    {
        try
        {
            grd = marbleGradient(0);
        }
        catch (err)
        {
            grd = 'blue';
        }
    }


    return grd;
}


/*
 *  Here we use the distance formula for some collision detection. 
 */
function squareRange(myObject)
{
    var distX = Math.abs(marble1.posX - myObject.posX);

    var distY = Math.abs(marble1.posY - myObject.posY);

    var actualDistance = Math.sqrt(Math.pow(distX, 2) + Math.pow(distY, 2))
    var touchDistance;
    if (Math.abs(marble1.myX) > 0 && Math.abs(marble1.myY) > 0) touchDistance = marble1.radius + myObject.width * (Math.sqrt(2) / 2);
    else touchDistance = marble1.radius + myObject.width * (Math.sqrt(2) / 2);

    if (actualDistance < touchDistance) return true;
    else return false;
}

/*
 *  Detects collision between marble and goal
 */
function checkGoal()
{
    var distX = Math.abs(marble1.posX - myGoal.posX)
    var distY = Math.abs(marble1.posY - myGoal.posY);

    if (distX < tileDim * 5 || distY < tileDim * 5) if (circleRange(myGoal))
    {
        myScore += 50;
        endLevel('Level ' + myLevel + ' Complete!');
        myLevel++;
    }

}

/*
 *  Collision detection between round objects
 */
function circleRange(myObject)
{
    var distX = Math.abs(marble1.posX - myObject.posX);

    var distY = Math.abs(marble1.posY - myObject.posY);


    if (Math.sqrt(Math.pow(distX, 2) + Math.pow(distY, 2)) < (marble1.radius + myObject.radius)) return true;
    else return false;
}

/*
 *  Collision detection with game elements
 */

// coins!
function coinTest()
{
    for (var e = 0; e < myCoins.length; e++)
    {

        var myObject = myCoins[e];

        myObject.draw();

        if (circleRange(myObject)) getCoin(myObject);
    }
}

// flames!
function flameTest()
{
    for (var e = 0; e < myFlames.length; e++)
    {
        var myObject = myFlames[e];

        myObject.draw();

        if (circleRange(myObject))
        {
            takeDamage(myObject);
            return true;
        }
    }
    return false;
}

// process whereby player obtains a coin. 
function getCoin(myObject)
{
    if (marble1)
    {
        myScore += myObject.value;

        // player slows down temporarily
        marble1.speed = 0.5;

        var slowTime = window.setTimeout(function ()
        {

            if (marble1) marble1.speed = 1;

        }, 1000)
        myObject.taken = true;

        eraseMe(myObject, 1, context)

        updateScore();
    }
}

/*
 *  Clears a part of the canvas that the object is on.
 */

function eraseMe(myObject, tiles, ctx)
{
    var dia = myObject.radius * 2;

    var pX = myObject.posX - myObject.radius * tiles;
    var pY = myObject.posY - myObject.radius * tiles;

    ctx.clearRect(pX, pY, dia * tiles, dia * tiles);
}

/*
 *  reduces health when colliding with flame
 */
function takeDamage(myObject)
{
    if (!marble1.dt)
    {
        marble1.dt = true;
        marble1.health--;
        if (marble1.health <= 0)
        {
            loseLife();
        }

        var timer = setTimeout(function ()
        {

            if (marble1) marble1.dt = false;
        }, 1000);
    }

    updateHealth();

}
/*
 *  gradient functions
 */

function marbleGradient(colorCode)
{
    var grd = context.createRadialGradient(marble1.posX, marble1.posY, 0, marble1.posX, marble1.posY, tileDim * 0.4);

    switch (colorCode)
    {
        case 0:
            grd.addColorStop(0, '#8ED6FF');
            grd.addColorStop(1, '#004CB3');
            break;

        case 1:
            grd.addColorStop(0, '#edafaf');
            grd.addColorStop(1, '#ea1c1c');

            break;
    }

    return grd;

}

function coinGradient(colorCode, myX, myY)
{
    // console.log('cg');
    var radius = tileDim * 0.4;
    var left = myX - radius;
    var right = myX + radius;
    var mid = myY

    var grd = context.createLinearGradient(left, mid, right, mid);


    switch (colorCode)
    {
        case 0:
            grd.addColorStop(0, 'yellow');
            grd.addColorStop(1, 'white');
            break;

        case 1:
            grd.addColorStop(0, 'green');
            grd.addColorStop(1, 'white');

            break;
    }


    return grd;

}


/*
 *  ends a level; used for game over and progressing to another level
 */

function endLevel(myMessage)
{
    paused = true;

    eraseMe(marble1, 3, contextMarble);

    // set drawn of goal to false so it will be drawn again. 
    myGoal.drawn = false;

    context.clearRect(0, 0, canvas.width, canvas.height);

    contextMarble.clearRect(0, 0, canvas.width, canvas.height);

    clearBoard();

    var gamescreen = document.getElementById('game_screen');
    var menuscreen = document.getElementById('level_complete');
    var message = document.getElementById('complete');

	menuscreen.style.width = window.innerWidth;
    gamescreen.style.visibility = 'collapse';

    message.innerHTML = myMessage;
	
	




    if (myMessage == 'Game Over')
    {
        gameOver();
    }
    else
    {
		
        // show menu screen
        menuscreen.style.visibility = 'inherit';

        var startTimer = window.setTimeout(function ()
        {

            gameStart();

        }, 1000);

       
    }



   

}

/* Game over function
 *
 */
function gameOver()
{




    var gameOver = document.getElementById('game_over');

    gameOver.style.visibility = 'inherit';

    // show data
    var endscore = document.getElementById('endscore');

    var endlevel = document.getElementById('endlevel');

    var endtime = document.getElementById('endtime');

    endscore.innerHTML = 'Score: ' + myScore;

    endlevel.innerHTML = 'Level: ' + myLevel;

    endtime.innerHTML = 'Time: ' + updateTime();

    // nav listeners

    var go_scores = document.getElementById('go_scores');

    var go_new = document.getElementById('go_new');

    var go_quit = document.getElementById('go_quit');

    sListener(go_scores);

    qListener(go_quit);

    nListener(go_new)





}

function loseLife()
{
    marble1.lives--;
    marble1.dt = 0;
    eraseMe(marble1, 3, contextMarble);

    if (marble1.lives <= 0)
    {
        storeRecord(null);
        endLevel('Game Over');



    }
    else
    {
        marble1.health = marble1.maxHealth;
        marble1.posX = marble1.originX;
        marble1.posY = marble1.originY;
    }
    updateHealth();
}

function testRecord()
{
    var key = 'scoreObjList';

    var myList = localStorage.getItem(key);

    if (myList) return true;
    else return false;
}

// converts a string time to a numerical value
function toSeconds(time)
{
    var minutes;

    if (time.length == 4)

    minutes = time.substring(time.length - 4, time.length - 3);
    else // if minutes are greater than 10
    minutes = time.substring(time.length - 5, time.length - 3);

    var seconds = time.substring(time.length - 2, time.length);

    minutes = parseInt(minutes);

    seconds = parseInt(seconds) + minutes * 60;

    return seconds;
}

function scanRecord()
{

    var storedScore = localStorage.getItem('scoreKey0');

    if (storedScore >= 0 && storedScore != null) return true;
    else return false;
}

function searchRecords(myLoc, myTime)
{
    // // console.log('searching records');
    var index = 0;

    for (var i = 0; i < 5; i++)
    {
        // get the variables from storage
        var storedSec = 0;
        var storedScore = localStorage.getItem('scoreKey' + i);
        var storedTime = localStorage.getItem('timeKey' + i);


        if (storedTime != null) storedSec = toSeconds(storedTime);

        var mySec = toSeconds(myTime);



        // if new score > existing score, put it there and bump down others.
        if (storedScore != null && (myScore > storedScore || (myScore == storedScore && mySec < storedSec)))
        {

            // bump this (and the scores after it) down
            moveScores(i, myLoc, myTime);

            // return this index
            return i;
        }

        // return a null value if new score isn't in top 5
        else if (myScore <= storedScore && i == 4)
        {

            return null;
        }
        else if (storedScore == null) // nothing below this, nothing to move
        {
            // don't move anything since there are no scores below it. 
            return i;
        }
    }
    // // console.log('this score is not in top five');

}

function printScores()
{


    for (var i = 0; i < 5; i++)
    {
        var oldScore = localStorage.getItem('scoreKey' + i);

        var oldTime = localStorage.getItem('timeKey' + i);

        var oldLoc = localStorage.getItem('locKey' + i);


        
    }

}

/*
 *  Pushes scores down, if a higher score is added. 
 */
function moveScores(start, myLoc, myTime)
{

    for (j = start; j < 4; j++)
    {
        // working backwards here. 
        // start with last score so we don't have score replication
        // this way i could be 3, 2, 1, or 0 - we always start with the furthest one down.
        currentRow = 3 - j;



        var oldScore = localStorage.getItem('scoreKey' + currentRow);

        var oldTime = localStorage.getItem('timeKey' + currentRow);

        var oldLoc = localStorage.getItem('locKey' + currentRow);
        // the next scoreKey
        var next = currentRow + 1;

        // move scores down
        if (oldScore >= 0)
        {
            // // console.log('storing row...');
            // put this value in the next key down
            storeRow(next, oldScore, oldTime, oldLoc);

        }

    }
}

function storeRecord(newSite)
{
    var myTime = updateTime();

    var myLoc = geo1.lat + ',' + geo1.long;

    var index = searchRecords(myLoc, myTime);
	
	try
	{
   	    connection.send(myScore);
	}
	catch (err)
	{
		console.log('error');
	}
    //connection.send(myScore + ' | ' + myTime + ' | ' + myLoc );

    // stores the row of data in its proper index. 
    if (index >= 0)
    {
        // store this row of data. 
        storeRow(index, myScore, myTime, myLoc);
    }

    // if a site was specified, navigate there. 				
    if (newSite) window.location.href = newSite;
}

/* stores a row of data in three localStorage objects with matching indices */
function storeRow(index, score, time, loc)
{
    localStorage.setItem('scoreKey' + index, score);

    localStorage.setItem('timeKey' + index, time);

    localStorage.setItem('locKey' + index, loc);

    return 0;
}

/*
 *   Stat Panel
 */
function updateLevel()
{
    document.getElementById('level').innerHTML = 'Level: ' + myLevel;
}

function updateScore()
{
    document.getElementById('score').innerHTML = 'Score: ' + myScore;

}

function updateHealth()
{
    document.getElementById('health').innerHTML = 'Health: ' + marble1.health;
    document.getElementById('lives').innerHTML = 'Lives: ' + marble1.lives;
}

function updateTime()
{
    var timeStat = document.getElementById('time');

    var gameMin = Math.floor(gameSeconds / 60);

    var gameSec = gameSeconds % 60;

    var secString = ('0' + gameSec).slice(-2);

    var min_sec = gameMin + ':' + secString;

    timeStat.innerHTML = 'Time: ' + min_sec;

    return min_sec;
}

/*
 *  Collision Detection
 */
function centerHit()
{

    for (var e = 0; e < myWalls.length; e++)
    {
        //// // // console.log("t: " + e);
        var myObject = myWalls[e];

        myObject.draw();

        var xd = Math.abs(myObject.posX - marble1.posX);
        var yd = Math.abs(myObject.posY - marble1.posY);

        if (yd < tileDim * 2.5 || xd < tileDim * 2.5) if (squareRange(myObject)) hitDirection(myObject);


    }


}


// this is how we set up a class. 
function yProximal(object1, object2)
{
    if (Math.abs(object1.posY - object2.posY) < object2.height / 2 + object1.radius) return true;
    else return false;
}
// compares x positions of marble and object
function xProximal(object1, object2)
{
    if (Math.abs(object1.posX - object2.posX) < object2.width / 2 + object1.radius) return true;
    else return false;
}


// determines whether to turn hit variables on or off
function hitDirection(object)
{
    // redraws the marble and wall
    marble1.draw();

    object.draw();

    // if marble is to the left and y positions are within proximity
    if (marble1.posX < object.posX && yProximal(marble1, object)) marble1.hitRight = true;

    else if (marble1.posX > object.posX && yProximal(marble1, object)) marble1.hitLeft = true;

    if (marble1.posY < object.posY && xProximal(marble1, object)) marble1.hitDown = true;

    else if (marble1.posY > object.posY && xProximal(marble1, object)) marble1.hitUp = true;

}

/*
 *  Geolocation
 */
function errorHandler(err)
{
    console.log('no location');
}

/*
 *  Instantiates geolocation object
 */
function getGeo()
{

    var geoloc = document.getElementById('geoloc');
				if (navigator.geolocation)
				{
					var options = {timeout:60000};
					navigator.geolocation.getCurrentPosition(showPosition, errorHandler, options);
				}
				else
				{
			
					//geoloc.innerHTML = 'Geolocation not supported';
				}
}

function showPosition(position)
{
    geo1.lat = parseInt(position.coords.latitude);
    geo1.long = parseInt(position.coords.longitude);
    
}

function geo(lat, long)
{
    this.lat = lat;
    this.long = long;
}




// marble class
function Marble(horiz, vert)
{
    // current velocity
    this.myX = 0;
    this.myY = 0;
  

    // starting location


    // damage
    this.dt = false;

    //lives & health
    this.lives = 3 //3;
    this.maxHealth = 2 //5;
    this.health = this.maxHealth;

    // starting location
    this.originX = horiz;
    this.originY = vert;

    // current location
    this.posX = this.originX;
    this.posY = this.originY;

    // radius of marble
    this.radius = tileDim * 0.4;

    // name of marble
    this.name = 'marble';

    this.speed = 1;

    this.hitUp = false;
    this.hitDown = false;
    this.hitRight = false;
    this.hitLeft = false;

}


// sets up a function for the marble class. 
Marble.prototype.draw = function (color)
{
    contextMarble.beginPath();
    contextMarble.arc(this.posX, this.posY, this.radius, 0, 2 * Math.PI, false);
    contextMarble.fillStyle = color;
    contextMarble.fill();
    contextMarble.lineWidth = 1;
    contextMarble.strokeStyle = 'black';
    contextMarble.stroke();
}







// makes sure marble is within screen
function testBounds(pos, pmin, pmax)
{
    pos = (pos < pmin) ? pmin : pos;

    pos = (pos > pmax) ? pmax : pos;

    return pos;
}

/*
 *  Moves the marble  based on position variables
 */
function moveMarble(myInterval)
{

    if (!marble1)
    {

        myInterval = window.clearInterval(myInterval);
        return 0;
    }


    if ((marble1.myX < 0 && marble1.hitLeft == false) || (marble1.myX > 0 && marble1.hitRight == false)) // if direction is up
    marble1.posX = marble1.posX + marble1.myX * marble1.speed;

    if ((marble1.myY < 0 && marble1.hitUp == false) || (marble1.myY > 0 && marble1.hitDown == false)) marble1.posY = marble1.posY + marble1.myY * marble1.speed;

    marble1.posX = testBounds(marble1.posX, leftBounds, rightBounds);

    marble1.posY = testBounds(marble1.posY, upperBounds, lowerBounds);

    clearHits();

}

function clearHits()
{
    marble1.hitRight = false;
    marble1.hitLeft = false;
    marble1.hitDown = false;
    marble1.hitUp = false;
}

