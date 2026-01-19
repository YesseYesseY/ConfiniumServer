namespace Globals
{
    static std::unordered_map<std::string, float> GameData;

    float GetGameData(const std::string& Key, float Default)
    {
        if (GameData.contains(Key))
        {
            return GameData[Key];
        }

        return Default;
    }

    float GetGameData(const FScalableFloat& SF, float Default)
    {
        return GetGameData(SF.Curve.RowName.ToString(), Default);
    }
}
