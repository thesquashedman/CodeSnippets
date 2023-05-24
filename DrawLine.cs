void DrawLine(InputInformation inp)
{
        if(inp.previousCanvasClick == inp.canvasClick)
    {
        return;
    }
    float yDistance = inp.previousCanvasClick.y - inp.canvasClick.y;
    float xDistance =  inp.previousCanvasClick.x - inp.canvasClick.x;
    float xOverY = Math.Abs(xDistance / yDistance) * (xDistance < 0 ? 1 : -1);
    float yOverX = Math.Abs(yDistance / xDistance) * (yDistance < 0 ? 1 : -1);

    //Check which of the distances is greater to determine when interpolating which direction (x or y) gets incremented by 1.
    //If the y is greater, then it gets incremented by 1, else the x gets incremented by 1
    if(Math.Abs(yDistance) >= Math.Abs(xDistance))
    {       
            int increment = (yDistance < 0) ? 1 : -1;
        float horizontalHeight = (float)(Math.Sqrt((xDistance * xDistance) + (yDistance * yDistance)) / Math.Abs(yDistance)) * inp.brushSize; //Length of horizontal strip.

        
        
        for(int i = 0; (i < Math.Abs(yDistance)); i++)
        {
            for(int j = 0; j < horizontalHeight/2; j++) ++)//Repeat for each row
            {
                Vector2 interpolationOffset = new Vector2(i * xOverY, i * increment);
                Vector2 brushOffset = new Vector2(j, 0);
                Vector2 pixelToSet = inp.previousCanvasClick + interpolationOffset + brushOffset;
                
                
                studentCanvas.SetPixel((int)pixelToSet.x, (int)pixelToSet.y, inp.brushColor);
                pixelToSet = inp.previousCanvasClick + interpolationOffset - brushOffset;
                studentCanvas.SetPixel((int)pixelToSet.x, (int)pixelToSet.y, inp.brushColor);
                
            }
        }
    }
    else{
        
        int increment = (xDistance < 0) ? 1 : -1;
        float verticalHeight = (float)(Math.Sqrt((xDistance * xDistance) + (yDistance * yDistance)) / Math.Abs(xDistance)) * inp.brushSize; //Length of vertical strip.

        for(int i = 0; (i < Math.Abs(xDistance)); i++)//Repeat for each column
        {
            for(int j = 0; j < verticalHeight/2; j++)
            {
                Vector2 interpolationOffset = new Vector2(i * increment, i * yOverX);
                Vector2 brushOffset = new Vector2(0, j);
                Vector2 pixelToSet = inp.previousCanvasClick + interpolationOffset + brushOffset;

                studentCanvas.SetPixel((int)pixelToSet.x, (int)pixelToSet.y, inp.brushColor);
                pixelToSet = inp.previousCanvasClick + interpolationOffset - brushOffset;
                studentCanvas.SetPixel((int)pixelToSet.x, (int)pixelToSet.y, inp.brushColor);
            }
            
        }
    }
    studentCanvas.Apply();     
}