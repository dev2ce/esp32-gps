const express = require("express");
const path = require("path");
const cors = require("cors");
const { readFileSync } = require("fs");
const { parse } = require("path");

const port = 8080;
const app = express();

app.use(cors());

const readGpsDataFromFile = function (req, res, next) {
	const contents = readFileSync(path.join(__dirname, "/data.txt"), "utf-8").toString();
	let arr = contents.split(",");

	let lat_natural = Math.floor(parseInt(arr[1])/100);
	let lat_decimal = (parseFloat(arr[1]) - lat_natural*100)/60.0;
	let lat = lat_natural + lat_decimal;

	let lng_natural = Math.floor(parseInt(arr[3])/100);
	let lng_decimal = (parseFloat(arr[3]) - lng_natural*100)/60.0;
	let lng = lng_natural + lng_decimal;
	
	res.json({ latitude: lat,  longtitude : lng});
};

app.get("/", function (req, res, next) {
	res.sendFile(path.join(__dirname, "/index.html"));
});

app.get("/gps_data", readGpsDataFromFile);

app.listen(port);

console.log("Server started at http://localhost:" + port);
