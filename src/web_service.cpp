#include <Arduino.h>
#include <web_serviece.h>

const char Web_Service::PAGE_AUTH[] PROGMEM = R"(
{
  "uri": "/auth",
  "title": "Auth",
  "menu": true,
  "element": [
    {
      "name": "text",
      "type": "ACText",
      "value": "AutoConnect has authorized",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    }
  ]
}
)";

const char Web_Service::file_log_page[] PROGMEM = R"raw(
[
  {
    "title": "SYS_LOG", "uri": "/file_log_page", "menu": true, "element": [
     {
        "name": "header",
        "type": "ACText",
        "value": "<h2>SYS_LOG_LIST</h2>",
        "style": "text-align:center; color:#2f4f4f; padding:10px;"
      },  
      {
        "name": "reset_set",
        "type": "ACRadio",
        "value": [
          "재시작",
          "재시작 안함"
        ], 
        "arrange": "horizontal",
        "checked": 1
      },
      {
        "name": "reset_time",
        "type": "ACInput",
        "label": "재시작 시간 :",
        "value": "10:00"
      },      
      {
        "name": "newline_1",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "period",
        "type": "ACRadio",
        "value": [
          "10 sec.",
          "30 sec.",
          "60 sec.",
          "180 sec."
        ],
        "label": "데이터 전송시간",
        "arrange": "vertical",
        "checked": 1
       },
      {
        "name": "newline_2",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "set_mode",
        "type": "ACRadio",
        "value": [
          "REAL_MODE",
          "TEST_MODE"
        ], 
        "arrange": "horizontal",
        "checked": 1
      }, 
      {
        "name": "newline_3",
        "type": "ACElement",
        "value": "<hr>"
      },
      
      {
        "name": "server_selsect",
        "type": "ACRadio",
        "value": [
          "로컬서버",
          "분석모델서버",
          "AWS서버"
        ], 
        "arrange": "horizontal",
        "checked": 1
      },
      
      {
        "name": "newline_4",
        "type": "ACElement",
        "value": "<hr>"
      },

      {
        "name": "ipaddress",
        "type": "ACInput",
        "label": "IP:",
        "pattern": "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$"
      },

      {
        "name": "portNum",
        "type": "ACInput",
        "label": "PORT :",
        "pattern": "^[0-9]{4}$"
      },
      {
        "name": "passwd",
        "type": "ACInput",
        "label": "PW :",
        "pattern": "^[a-zA-Z0-9]{8}$"
      },
      
      {
        "name": "send",
        "type": "ACSubmit",
        "value": "SEND",
        "uri": "/check"
      }
    ]
  },

  {
    "title": "IP Address",
    "uri": "/check",
    "menu": false,
    "element": [
      {
        "name": "result",
        "type": "ACText"
       
      }
    
    ]
  }
]
)raw";

const char Web_Service::input_sensor_page[] PROGMEM = R"(
[
 {
  "uri": "/select_sensor",
  "title": "센서설정",
  "menu": true,
  "element":
   [
     {
        "name": "header",
        "type": "ACText",
        "value": "<h2>센서등록설정</h2>",
        "style": "text-align:left;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "tablestyle",
        "type": "ACStyle",
        "value": "table.style
        {
          font-family:arial,sans-serif;
          border-collapse:collapse;
          width:100%;
          color:black;
        }
        table.style td,table.style th
        {
          border:1px solid #dddddd;
          text-align:left;
          padding:8px;
        }
        table.style tr:nth-child(even)
        {
          background-color:#ffffff;
        }"
      },
      
     {
        "name": "sensor",
        "type": "ACElement",
        "value": "<table class='style'>      
                    <tr>
                    <td>구 분</td>
                    <td>센서 종류</td>
                    <td>센서 코드</td>
                    <td>체크</td>
                    </tr>   
                    <tr>
                    <td rowspan = '5'>온/습도 센서</td>
                    <td>구형 온습도 센서</td>
                    <td> T , H </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_5'></td>
                    </tr>
                    <td>백엽상일체형 센서, 리카(RK330-01) 센서</td>
                    <td> J , K </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_1'></td>
                    </tr>
                    <tr>
                    <td>건/습구 센서</td>
                    <td> Y , L </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_2'></td>
                    </tr>
                    <tr>
                    <td>서리 온도/습도/노점온도 센서</td>
                    <td> U , r , d </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_3'></td>
                    </tr>
                    <tr>
                    <td>엽온 센서</td>
                    <td> l(소문자) </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_4'></td>
                    </tr>
                   
                    <tr>
                    <td rowspan = '3'>Co2 센서</td>
                    <td>Co2(KCD-AN300) 센서</td>
                    <td> C </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_2_1'></td>
                    </tr>
                    <tr>
                    <!--<td>Co2(KCD-AN300)</td>-->
                    <td>Co2(KCD-HP100) 센서</td>
                    <td> c </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_2_2'></td>
                    </tr>
                    <tr>
                    <!--<td>Co2(KCD-AN300)</td>-->
                    <td>Co2(SH-VT260) 센서</td>
                    <td> i , o , p </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_2_3'></td>
                    </tr>
                 
                    <tr>
                    <td rowspan = '5'>기상대 센서</td>
                    <td>일사량(코리아 디지털, SWSR-7500) 센서</td>
                    <td> G </td> 
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_1'></td>
                    </tr>
                    <tr>
                    <td>리카 풍향(RK110-02) 센서</td>
                    <td> V </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_2'></td>
                    </tr>
                    <tr>
                    <td>리카 풍속(RK100-02) 센서</td>
                    <td> X </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_3'></td>
                    </tr>
                    <tr>
                    <td>우적 센서</td>
                    <td> R </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_4'></td>
                    </tr>
                    <tr>
                    <td>코린스 센서</td>
                    <td>y,O,g,A,s,e,Q </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_5'></td>
                    </tr>

                    <tr>
                    <td rowspan = '2'>용액용 센서</td>
                    <td>리카 PH(RK500-02) 센서</td>
                    <td> j </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_4_1'></td>
                    </tr>
                    <tr>
                    <!--<td>Co2(KCD-AN300)</td>-->
                    <td>리카 EC,수온(RK500-13) 센서</td>
                    <td> x , 1 </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_4_2'></td>
                    </tr>
                                   
                    <tr>
                    <td rowspan = '3'>토지용 센서</td>
                    <td>리카 PH(RK500-02) 센서</td>
                    <td> j </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_5_1'></td>
                    </tr>
                    <tr>
                    <td>리카 EC/지온/지습(RK520-02) 센서</td>
                    <td> v , n , q </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_5_2'></td>
                    </tr>
                    <tr>
                    <td>미래 EC/지온/지습(RK520-02) 센서</td>
                    <td> E , m , u </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_5_3'></td>
                    </tr>

                    <tr>
                    <td rowspan = '5'>감지 센서</td>
                    <td>수위감지 센서</td>
                    <td> w </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_1'></td>
                    </tr>
                    <tr>
                    <td>창문감지 센서</td>
                    <td> N </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_2'></td>
                    </tr>
                    <tr>
                    <td>화재감지 센서</td>
                    <td> F </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_3'></td>
                    </tr>
                    <tr>
                    <td>정전알림 센서</td>
                    <td> b </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_4'></td>
                    </tr>
                    <tr>
                    <td>출입문감지 센서</td>
                    <td> M </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_5'></td>
                    </tr>

                    <tr>
                    <td rowspan = '2'>기타 센서</td>
                    <td>Battery 잔량체크 센서</td>
                    <td> B </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_7_1'></td>
                    </tr>
                    <tr>
                    <td>유량 체크 센서</td>
                    <td> f </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_7_2'></td>
                    </tr>

                    <tr>
                    <td>해 제</td>
                    <td>모든 설정센서 해제</td>
                    <td>없음</td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_8_1'></td>
                    </tr>

                  </table>"
      },

      {
        "name": "send",
        "type": "ACSubmit",
        "value": "SEND",
        "uri": "/Select_sensor_check"
      }
    ]
  },

 {
    "title": "Sensor_result",
    "uri": "/Select_sensor_check",
    "menu": false,
    "element": 
    [
      {
        "name": "result",
        "type": "ACText"
        
      },
      {
        "name": "result_2",
        "type": "ACText"
        
      }
    ]
  }
]
)";

const char Web_Service::input_page[] PROGMEM = R"raw(
[
  {
    "title": "환경설정", "uri": "/input_page", "menu": true, "element": [
     
     {
        "name": "header",
        "type": "ACText",
        "value": "<h2>환경 설정</h2>",
        "style": "text-align:center; color:#2f4f4f; padding:10px;"
      },  

     {
        "name": "reset_set",
        "type": "ACRadio",
        "value": [
          "재시작",
          "재시작 안함"
        ], 
        "arrange": "horizontal",
        "checked": 1
      }, 

      {
        "name": "reset_time",
        "type": "ACInput",
        "label": "재시작 시간 :",
        "value": "10:00"
      },      
     {
        "name": "newline_1",
        "type": "ACElement",
        "value": "<hr>"
      },

      {
        "name": "period",
        "type": "ACRadio",
        "value": [
          "10 sec.",
          "30 sec.",
          "60 sec.",
          "180 sec."
        ],
        "label": "데이터 전송시간",
        "arrange": "vertical",
        "checked": 1
       },
  
      {
        "name": "newline_2",
        "type": "ACElement",
        "value": "<hr>"
      },

      {
        "name": "set_mode",
        "type": "ACRadio",
        "value": [
          "REAL_MODE",
          "TEST_MODE"
        ], 
        "arrange": "horizontal",
        "checked": 1
      }, 
     
      {
        "name": "newline_3",
        "type": "ACElement",
        "value": "<hr>"
      },
      
      {
        "name": "server_selsect",
        "type": "ACRadio",
        "value": [
          "로컬서버",
          "분석모델서버",
          "AWS서버"
        ], 
        "arrange": "horizontal",
        "checked": 1
      },
      
      {
        "name": "newline_4",
        "type": "ACElement",
        "value": "<hr>"
      },

      {
        "name": "ipaddress",
        "type": "ACInput",
        "label": "IP:",
        "pattern": "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$"
      },

      {
        "name": "portNum",
        "type": "ACInput",
        "label": "PORT :",
        "pattern": "^[0-9]{4}$"
      },
      {
        "name": "passwd",
        "type": "ACInput",
        "label": "PW :",
        "pattern": "^[a-zA-Z0-9]{8}$"
      },
      
      {
        "name": "send",
        "type": "ACSubmit",
        "value": "SEND",
        "uri": "/check"
      }
    ]
  },

  {
    "title": "IP Address",
    "uri": "/check",
    "menu": false,
    "element": [
      {
        "name": "result",
        "type": "ACText"
       
      }
    
    ]
  }
]
)raw";

const char Web_Service::Sensor_cal[] PROGMEM = R"raw(
[
  { "title":"CAL설정", "uri":"/calibration", "menu":true, "element":[
    
      {
          "name": "tablestyle",
          "type": "ACStyle",
          "value": "table.style
          {
            font-family:arial,sans-serif;
            border-collapse:collapse;
            width:100%;
            color:black;
          }

          table.style td,table.style th
          {
            border:1px solid #dddddd;
            text-align:left;
            padding:8px;
          }

          table.style tr:nth-child(even)
          {
            background-color:#ffffff;
          }"
      },

      {
        "name": "header",
        "type": "ACText",
        "value": "<h3>RK500-02_PH_CAL</h3>",
        "style": "text-align:left;color:#2f4f4f;padding:5px;"
      },

      {
        "name": "PH_SENSOR_CAL",
        "type": "ACElement",
        "value": "<table class='style'>      
                   <tr>
                    <td rowspan = '4'>PH</td>
                    <td>현재 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(1);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_1' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>9.18 용액 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(2);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_2' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='button' id='button1' onclick='ph_918_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>6.86 용액 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(3);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_3' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='button' id='button1' onclick='ph_686_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>4.00 용액 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(4);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_4' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='button' id='button1' onclick='ph_400_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                  </table>"
              
      },
     {
        "name": "header_1",
        "type": "ACText",
        "value": "<h3>RK500-13_EC_CAL</h3>",
        "style": "text-align:left;color:#2f4f4f;padding:5px;"
      },
     {
        "name": "EC_SENSOR_CAL",
        "type": "ACElement",
        "value": "<table class='style'>      
                   
                   <tr>
                    <td rowspan = '4'>EC</td>
                    <td>현재값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ec_read();' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>PV_EC</td>
                    <td>PV_T</td>
                    <td>PV_C1</td>
                    <td>PV_C2</td>
                    </tr>

                    <tr>
                    <td><input type='text' value='0' id='EC_answer_1' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='EC_answer_2' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='EC_answer_3' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='EC_answer_4' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>CAL</td>
                    <td><input type='button' id='button1' onclick='ec_1413_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td colspan='2'><input type='text' value='0' id='EC_answer_5' size='10' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>
                   
                  </table>"
              
      },
      {
        "name": "js",
        "type": "ACElement",
        "value": "<script type='text/javascript'>    
                  
                  var PV_ph_read_result;
                  var ph_918_handler; 
                  var ph_686_handler; 
                  var ph_400_handler;

                  var PV_ec_read_result;  
                  var ec_1413_handler; 

                  function PV_ph_read(a)
                   {
                      PV_ph_read_result = new XMLHttpRequest();
                      PV_ph_read_result.onreadystatechange=function()
                      {
                        if(this.readyState==4 && PV_ph_read_result.status==200)
                          { 
                           if(a == '1') document.getElementById('PH_answer_1').value=this.responseText;                          
                           if(a == '2') document.getElementById('PH_answer_2').value=this.responseText;                          
                           if(a == '3') document.getElementById('PH_answer_3').value=this.responseText;                          
                           if(a == '4') document.getElementById('PH_answer_4').value=this.responseText;                          
                          }
                      }
                      PV_ph_read_result.open('GET', '/PH_PV_read');
                      PV_ph_read_result.send();                  
                    }
  
                  function ph_918_cal_compleat() 
                  {
                      ph_918_handler = new XMLHttpRequest();
                      ph_918_handler.onreadystatechange=function()
                      {
                        if(this.readyState==4 && ph_918_handler.status==200)
                          { 
                            document.getElementById('PH_answer_2').value=this.responseText; 
                          }                         
                      }
                      ph_918_handler.open('GET', '/PH_918_result');
                      ph_918_handler.send();                  
                    }
                 
                  function ph_686_cal_compleat()
                   {
                      ph_686_handler = new XMLHttpRequest();
                      ph_686_handler.onreadystatechange=function()
                      {
                        if(this.readyState==4 && ph_686_handler.status==200)
                          { 
                            document.getElementById('PH_answer_3').value=this.responseText; 
                          }                         
                      }
                      ph_686_handler.open('GET', '/PH_686_result');
                      ph_686_handler.send();       
                    } 
                  
                  function ph_400_cal_compleat() 
                  {
                    ph_400_handler = new XMLHttpRequest();
                    ph_400_handler.onreadystatechange=function()
                    {
                      if(this.readyState==4 && ph_400_handler.status==200)
                        { 
                          document.getElementById('PH_answer_4').value=this.responseText; 
                        }                         
                    }
                    ph_400_handler.open('GET', '/PH_400_result');
                    ph_400_handler.send();       
                    }

                   function PV_ec_read()
                   {
                      PV_ec_read_result = new XMLHttpRequest();
                      PV_ec_read_result.onreadystatechange=function()
                      {
                        if(this.readyState==4 && PV_ec_read_result.status==200)
                          { 
                            var str = this.responseText;
                            var arr = str.split(',');
                            document.getElementById('EC_answer_1').value = arr[0];
                            document.getElementById('EC_answer_2').value = arr[1];
                            document.getElementById('EC_answer_3').value = arr[2];
                            document.getElementById('EC_answer_4').value = arr[3];
                          }
                      }
                      PV_ec_read_result.open('GET', '/EC_PV_read');
                      PV_ec_read_result.send();                  
                    }
                 

                  function  ec_1413_cal_compleat() {
                    ec_1413_handler = new XMLHttpRequest();
                    ec_1413_handler.onreadystatechange=function()
                    {
                      if(this.readyState==4 && ec_1413_handler.status==200)
                        { 
                          document.getElementById('EC_answer_5').value=this.responseText; 
                        }                         
                    }
                    ec_1413_handler.open('GET', '/EC_1413_result');
                    ec_1413_handler.send();       
                    }

                    </script>"
      }
    
    ]
  }
]
)raw";

const char Web_Service::auxPage[] PROGMEM = R"raw(
[
  { "title":"로그", "uri":"/echo", "menu":true, "element":[
    
    { "name":"view_time", "type":"ACElement", "value":"<div id='time'>시간 대기 중...</div>" },
    { "name":"view_real_time_data", "type":"ACElement", "value":"<div id='status_log_real_time_data'>센서 데이터 대기 중...</div>" },
    { "name":"view_status_log", "type":"ACElement", "value":"<div id='status_log'>전송 데이터 대기 중...</div>" },
    { "name":"view_post_code", "type":"ACElement", "value":"<div id='post_code_check'>POST 상태 대기 중...</div>" },
    { "name":"view_get_code", "type":"ACElement", "value":"<div id='get_code_check'>GET 상태 대기 중...</div>" },
    { "name":"view_heap", "type":"ACElement", "value":"<div id='status_log_heap'>메모리 상태 대기 중...</div>" },
    
    {
    "name": "js_st",
    "type": "ACElement",
    "value": "<script type='text/javascript'>
    
    var xhr_get_code_check;
    var xhr_post_code_check;
    var xhr_st_real_time_data;
    var xhr_heap;
    var xhr_st;
    var xhr;

    function get_code_check(){
      xhr_get_code_check.open('GET', '/get_code_check');
      xhr_get_code_check.responseType='text';
      xhr_get_code_check.send();
      }
    
    function post_code_check(){
      xhr_post_code_check.open('GET', '/post_code_check');
      xhr_post_code_check.responseType='text';
      xhr_post_code_check.send();
      }

    function status_log_real_time_data(){
      xhr_st_real_time_data.open('GET', '/status_log_real_time_data');
      xhr_st_real_time_data.responseType='text';
      xhr_st_real_time_data.send();
      }

    function status_log_heap(){
      xhr_heap.open('GET', '/status_log_heap');
      xhr_heap.responseType='text';
      xhr_heap.send();
      }
    
    function status_log(){
      xhr_st.open('GET', '/status_log');
      xhr_st.responseType='text';
      xhr_st.send();
      }
    
    function clock(){
      xhr.open('GET', '/clock');
      xhr.responseType='text';
      xhr.send();
      }  

    window.onclose=function(){
      xhr_get_code_check.abort();
      xhr_post_code_check.abort();
      xhr_st_real_time_data.abort();
      xhr_heap.abort();
      xhr_st.abort();
      xhr.abort();
      };

    window.addEventListener('load', function()
    {
        xhr_post_code_check=new XMLHttpRequest();
        xhr_post_code_check.onreadystatechange=function(){
          if(this.readyState==4&&xhr_post_code_check.status==200){
            var target=document.getElementById('post_code_check');
            if(target) target.textContent=this.responseText;
            }
          };
          setInterval(post_code_check,5000);

        xhr_get_code_check=new XMLHttpRequest();
        xhr_get_code_check.onreadystatechange=function(){
          if(this.readyState==4&&xhr_get_code_check.status==200){
            var target=document.getElementById('get_code_check');
            if(target) target.textContent=this.responseText;
            }
          };
          setInterval(get_code_check,5000);


        xhr_st_real_time_data=new XMLHttpRequest();
        xhr_st_real_time_data.onreadystatechange=function(){
          if(this.readyState==4&&xhr_st_real_time_data.status==200){
            var target=document.getElementById('status_log_real_time_data');
            if(target) target.textContent=this.responseText;
            }
          };
          setInterval(status_log_real_time_data,1000);
          
        xhr_heap=new XMLHttpRequest();
        xhr_heap.onreadystatechange=function(){
          if(this.readyState==4&&xhr_heap.status==200){
            var target=document.getElementById('status_log_heap');
            if(target) target.textContent=this.responseText;
            }
          };
          setInterval(status_log_heap,3000);

        xhr_st=new XMLHttpRequest();
        xhr_st.onreadystatechange=function(){
          if(this.readyState==4&&xhr_st.status==200){
            var target=document.getElementById('status_log');
            if(target) target.textContent=this.responseText;
            }
          };
          setInterval(status_log,2000);

        xhr=new XMLHttpRequest();
        xhr.onreadystatechange=function(){
          if(this.readyState==4&&xhr.status==200){
            var target=document.getElementById('time');
            if(target) target.textContent=this.responseText;
            }
          };
        status_log_real_time_data();
        status_log_heap();
        status_log();
        post_code_check();
        get_code_check();
        clock();

        setInterval(clock,2000);
      });

    </script>"},

    { "name":"echo", "type":"ACText" },
    { "name":"echo_1", "type":"ACText" },
    { "name":"echo_2", "type":"ACText" }

    ]
  }
]
)raw";
