$(document).ready(function(){
	var machine4 = $("#casino1").slotMachine({
		active	: 0,
		delay	: 500,
		randomize : function(activeElementIndex){
			return 0;
		}
	});

	var machine5 = $("#casino2").slotMachine({
		active	: 1,
		delay	: 500,
		randomize : function(activeElementIndex){
			return 0;
		}
	});

	var machine6 = $("#casino3").slotMachine({
		active	: 2,
		delay	: 500,
		randomize : function(activeElementIndex){
			return 0;
		}
	});

	var machine7 = $("#casino4").slotMachine({
		active	: 3,
		delay	: 500,
		randomize : function(activeElementIndex){
			return 0;
		}
	});

	var started = 0;

	$("#slotMachineButtonShuffle").click(function(){
		started = 4;
		machine4.shuffle();
		machine5.shuffle();
		machine6.shuffle();
		machine7.shuffle();
	});

	$("#slotMachineButtonStop").click(function(){
		httpService.getCachelineOffset();
		switch(started){
			case 4:
				machine7.stop();
				break;
			case 3:
				machine4.stop();
				break;
			case 2:
				machine5.stop();
				break;
			case 1:
				machine6.stop();
				break;
		}
		started--;
	});
});
	