namespace GetFile
{
    class Program
    {
        static void Main()
        {
            string urlAddress = "http://2016.eicar.org/download/eicar.com";
            System.Net.WebClient client = new System.Net.WebClient();
            client.DownloadFile(urlAddress, @"eicar.com");
        }
    }
}
