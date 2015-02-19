// JavaScript Document
init();

function init() 
{
	// listener for new game

	window.setTimeout(function () { addListeners() }, 1000);
}
	

function addListeners()
{	
	var hiScores = document.getElementById('main_scores');
	
		
	scoreButton(hiScores);
	
	var newGame =  document.getElementById('main_new');
	
	var quit = document.getElementById('main_quit');

	newGame.addEventListener('touchstart', function (event)
	{
		window.location.href = 'http://birdleymedia.com/e76/game3.html';
	});
	
	// listener for scores
	
	hiScores.addEventListener('touchstart', function (event)
	{
		window.location.href = 'http://birdleymedia.com/e76/scores.html';
	});
	// listener for quit
	
	quit.addEventListener('touchstart', function (event)
	{
		window.location.href = 'http://birdleymedia.com';
	});
	
	
	/** mouse events */
	
	newGame.addEventListener('mousedown', function (event)
	{
		window.location.href = 'http://birdleymedia.com/e76/game3.html';
	});
	
	// listener for scores
	
	hiScores.addEventListener('mousedown', function (event)
	{
		window.location.href = 'http://birdleymedia.com/e76/scores.html';
	});
	
	// listener for quit
	quit.addEventListener('mousedown', function (event)
	{
		window.location.href = 'http://birdleymedia.com';
	});
}

function scoreButton(hiScores)
{
		if (scanRecord)
			hiScores.style.visibility = 'inherit';
		else
			hiScores.style.visibility = 'collapse';				
}
function scanRecord()
{	
		var storedScore = localStorage.getItem('scoreKey0');
					
		if (storedScore >=0)
			return true;
		else
			return false;
}