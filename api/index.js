const express = require("express")

const app = express();
const PORT = 8080;

app.get("/user/var/120/10221/0/0/12197", (req,res) => {
	res
	.status(200)
	.send('<eta version="1.0"><value uri="/user/var/120/10221/0/0/12197" strValue="10,1" unit="°C" decPlaces="1" scaleFactor="10" advTextOffset="0">101</value></eta>');
});

app.get("/user/var/120/10221/0/0/12185", (req,res) => {
	res
	.status(200)
	.send('<eta version="1.0"><value uri="/user/var/120/10221/0/0/12185" strValue="35" unit="°C" decPlaces="0" scaleFactor="10" advTextOffset="0">346</value></eta>');
});

app.get("/user/var/120/10221/0/0/12275", (req,res) => {
	res
	.status(200)
	.send('<eta version="1.0"><value uri="/user/var/120/10221/0/0/12275" strValue="37" unit="°C" decPlaces="0" scaleFactor="10" advTextOffset="0">370</value></eta>');
});

app.get("/user/var/120/10221/0/0/12278", (req,res) => {
	res
	.status(200)
	.send('<eta version="1.0"><value uri="/user/var/120/10221/0/0/12278" strValue="0" unit="%" decPlaces="0" scaleFactor="10" advTextOffset="0">0</value></eta>');
});


app.get("*", (req,res) => {
	res.sendStatus(404);
});

app.listen(PORT, () => {
	console.log(`Server is running on port ${PORT}.`)
});