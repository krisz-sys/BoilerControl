const char index_html[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>ESP8266 Slider</title>
    <style>
      body { font-family: sans-serif; text-align: center; margin-top: 50px; }
	  .sliderStyle { -webkit-appearance: none; margin: 14px; width: 360px; height: 20px; background: #d3d3d3; border-radius: 10px;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
	.sliderStyle::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; border-radius: 10px; background: #04AA6D ;  cursor: pointer;}
    .sliderStyle::-moz-range-thumb { width: 35px; height: 35px; border-radius: 10px; background: #04AA6D; cursor: pointer; } 
     input[type=range] { width: 50%; }
	  
        table, th, td {
          border-collapse: collapse;
		  border: 3px solid #cccccc;
          text-align: center;
        }
		table.center {
		  margin-left: auto; 
		  margin-right: auto;
		}
        .content {
            padding: 10px;
            margin: 10px 0;
            border: 0px solid #ccc;
            border-radius: 4px;
            width: 200px;
            transition: border-color 0.3s ease;
        }

</style>
	  
    </style>
  </head>
  <body>
    <h2>Live Slider Update</h2>
    <input type="range" id="slider" min="0" max="1200" value="0" class="sliderStyle">
    <p>Value: <span id="value">0</span></p>
    <p><span id="Temp"></span></p>
    <p><span id="HeaterOn"></span></p>
    <p><span id="test"></span></p>
	
	<button id="myBtn" value="ON" onclick="ButtonFunction()">D2 ON</button><br><br><br>


	<table class="center">
  <tr>
    <th>Nr.</th>
    <th>Hours & Minutes</th>
    <th>Time(min)</th>
  </tr>
  <tr>
    <td>1</td>
    <td><input type="time" id="TS1_H" name="appt" class="content"></td>
    <td><input type="number" id="TS1_T" name="quantity" min="0" max="30" class="content"></td>
  </tr>
  <tr>
    <td>2</td>
    <td><input type="time" id="TS2_H" name="appt" class="content"></td>
    <td><input type="number" id="TS2_T" name="quantity" min="0" max="30" class="content"></td>
  </tr>
  <tr>
    <td>3</td>
    <td><input type="time" id="TS3_H" name="appt" class="content"></td>
    <td><input type="number" id="TS3_T" name="quantity" min="0" max="30" class="content"></td>
  </tr>

</table>

	

	<img src="data:image/jpeg;base64,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" width="50%" height="50%"/>


    <script>
		function ButtonFunction() {
			  var x = button.value;
			  if(x=="ON"){
				button.value="OFF";
				button.innerText="D2 OFF";
			  }
			  else{
				button.value="ON";
				button.innerText="D2 ON";
			  }
			  console.log(button.value);
		}
	
      const slider = document.getElementById('slider');
      const valueLabel = document.getElementById('value');
      const temperature=document.getElementById('Temp');
      const heaterOn=document.getElementById('HeaterOn');
      const test=document.getElementById('test');
      const TS1_H=document.getElementById('TS1_H');
      const TS1_T=document.getElementById('TS1_T');
      const TS2_H=document.getElementById('TS2_H');
      const TS2_T=document.getElementById('TS2_T');
      const TS3_H=document.getElementById('TS3_H');
      const TS3_T=document.getElementById('TS3_T');
	  const button=document.getElementById("myBtn");
      const ws = new WebSocket('ws://' + window.location.host + '/ws');
	  
	  
	  

      ws.onmessage = (event) => {										// geting data from ESP8266
        valueLabel.textContent = event.data;
        const myArray = event.data.split(", ");
        temperature.textContent=myArray[1]+" °C";

        let sliderValue = Number(myArray[0].trim());
        if(sliderValue<0){
			    sliderValue=0;
		    }
        test.textContent=myArray[0].trim();
        slider.value=sliderValue.toString();
		    const seconds=Number(myArray[0])*3;
        const minutes=(seconds-(seconds%60))/60;
		
        heaterOn.textContent='Boiler is ON for '+minutes.toString()+' min, and '+(seconds%60).toString()+' s!';
        //heaterOn.textContent=seconds.toString();
        slider.value = seconds.toString();
		
        TS1_H.value=myArray[2].trim();
        TS1_T.value=myArray[3].trim();
        
        TS2_H.value=myArray[4].trim();
        TS2_T.value=myArray[5].trim();
        
        TS3_H.value=myArray[6].trim();
        TS3_T.value=myArray[7].trim();
		
      };

      slider.addEventListener('input', () => {					// sending data to ESP8266
        valueLabel.textContent = slider.value;
        const seconds=Number(slider.value);
        const minutes=(seconds-(seconds%60))/60;
        let sliderValue=slider.value/3;
        if(sliderValue<0){
              sliderValue=0;
        }
        heaterOn.textContent='Boiler is ON for '+minutes.toString()+' min, and '+(seconds%60).toString()+' s!';
		const msg="Slider: "+ sliderValue.toString()+", Button: "+button.value;
		console.log(msg);
        ws.send(msg);
      });
	  
	  button.addEventListener('click', () => {
			const msg="Button: "+button.value;
			console.log(msg);
			ws.send(msg);
		});
	
		TS1_H.addEventListener('input', () => {
			const msg="TS1_H: "+TS1_H.value;
			console.log(msg);
			ws.send(msg);
		});
	
		TS1_T.addEventListener('input', () => {
			const msg="TS1_T: "+TS1_T.value;
			console.log(msg);
			ws.send(msg);
		});
		
		TS2_H.addEventListener('input', () => {
			const msg="TS2_H: "+TS2_H.value;
			console.log(msg);
			ws.send(msg);
		});
	
		TS2_T.addEventListener('input', () => {
			const msg="TS2_T: "+TS2_T.value;
			console.log(msg);
			ws.send(msg);
		});
		
		TS3_H.addEventListener('input', () => {
			const msg="TS3_H: "+TS3_H.value;
			console.log(msg);
			ws.send(msg);
		});
	
		TS3_T.addEventListener('input', () => {
			const msg="TS3_T: "+TS3_T.value;
			console.log(msg);
			ws.send(msg);
		});
	  
	  

    </script>
  </body>
</html>
)=====";