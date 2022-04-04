const char main_page_pre[] PROGMEM = R"=====(
<HTML>

<HEAD>
    <TITLE>My first web page</TITLE>
    <style>
        table, th, td {
            border: 1px solid black;
        }

        .row {
            margin: 10px;
            padding: 10px;
        }

        .cell_div {
            margin: 5px;
            padding: 5px;
        }
    </style>
</HEAD>

<BODY>
    <form action="/update_colors" method="POST">
        <div class="row">
            <label for="converter">Converter LED</label>
)=====";
            <input type="color" id="converter" name="converter" value="#e66465">

const char main_page_table_start[] PROGMEM = R"=====(
        </div>
        <table class="row">
)=====";
            <tr>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_seven" name="led_seven" value="#e66465">
                    </div>
                </td>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_six" name="led_six" value="#e66465">
                    </div>
                </td>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_one" name="led_one" value="#e66465">
                    </div>
                </td>
            </tr>
            <tr>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_eight" name="led_eight" value="#e66465">
                    </div>
                </td>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_five" name="led_five" value="#e66465">
                    </div>
                </td>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_two" name="led_two" value="#e66465">
                    </div>
                </td>
            </tr>
            <tr>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_nine" name="led_nine" value="#e66465">
                    </div>
                </td>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_four" name="led_four" value="#e66465">
                    </div>
                </td>
                <td>
                    <div class="cell_div">
                        <input type="color" id="led_three" name="led_three" value="#e66465">
                    </div>
                </td>
            </tr>

const char main_page_post[] PROGMEM = R"=====(
		</table>
        <input type="submit" value="Update Colors" class="row">
    </form>
    
</BODY>

</HTML>
)=====";