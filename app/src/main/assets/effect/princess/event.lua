
local Sticker2DV3 = {
playClip = function (this, path, entityName, clipName, playTimes)
    local feature = this:getFeature(path)
    local feature_2dv3 = EffectSdk.castSticker2DV3Feature(feature)
    if (feature_2dv3) then
        feature_2dv3:resetClip(entityName, clipName)
        feature_2dv3:playClip(entityName, clipName, -1, playTimes)
    end
end,
stopClip = function (this, path, entityName, clipName)
    local feature = this:getFeature(path)
    local feature_2dv3 = EffectSdk.castSticker2DV3Feature(feature)
    if (feature_2dv3) then
        feature_2dv3:resumeClip(entityName, clipName, false)
        feature_2dv3:appearClip(entityName, clipName, false)
        feature_2dv3:resetClip(entityName, clipName)
    end
end,
playLastClip = function (this, path, entityName, clipName)
    local feature = this:getFeature(path)
    local feature_2dv3 = EffectSdk.castSticker2DV3Feature(feature)
    if (feature_2dv3) then
        feature_2dv3:appearClip(entityName, clipName, true)
    end
end,
}

local FaceMakeupFunc = {
playMakeup = function (this, path, type)
    local feature = this:getFeature(path)
    local feature_makeup = EffectSdk.castFaceMakeupV2Feature(feature)
    if (feature_makeup) then
        feature_makeup:show(type)
        feature_makeup:seek(type, 0, 0)
        feature_makeup:play(type)
    end
end,
showMakeup = function (this, path, type)
    local feature = this:getFeature(path)
    local feature_makeup = EffectSdk.castFaceMakeupV2Feature(feature)
    if (feature_makeup) then
        feature_makeup:show(type)
    end
end,
hideMakeup = function (this, path, type)
    local feature = this:getFeature(path)
    local feature_makeup = EffectSdk.castFaceMakeupV2Feature(feature)
    if (feature_makeup) then
        feature_makeup:hide(type)
    end
end,
pauseMakeup = function (this, path, type)
    local feature = this:getFeature(path)
    local feature_makeup = EffectSdk.castFaceMakeupV2Feature(feature)
    if (feature_makeup) then
        feature_makeup:pause(type)
    end
end,
}

local CommonFunc = {
setFeatureEnabled = function (this, path, status)
    local feature = this:getFeature(path)
    if (feature) then
        feature:setFeatureStatus(EffectSdk.BEF_FEATURE_STATUS_ENABLED, status)
    end
end,
}
local init_state = 1
local timer_id_787 = 101
local timer_id_790 = 201
local timer_id_793 = 301
local timer_id_796 = 401
local timer_id_798 = 501
EventHandles = {
    handleEffectEvent = function (this, eventCode)
        if(init_state == 1) then 
            init_state = 0
            if (eventCode == 1) then
                Sticker2DV3.stopClip(this, "2DStickerV3_5101", "entityname8D74E2470C544C0E9561CCE5B3C2026C", "clipname1")
                Sticker2DV3.stopClip(this, "2DStickerV3_5101", "entityname8D74E2470C544C0E9561CCE5B3C2026C", "clipname2")
                Sticker2DV3.stopClip(this, "2DStickerV3_5101", "entityname8D74E2470C544C0E9561CCE5B3C2026C", "clipname3")
                Sticker2DV3.playClip(this, "2DStickerV3_5101", "entityname8D74E2470C544C0E9561CCE5B3C2026C", "clipname1", 0)
                Sticker2DV3.playClip(this, "2DStickerV3_5101", "entityname8D74E2470C544C0E9561CCE5B3C2026C", "clipname2", 0)
                Sticker2DV3.playClip(this, "2DStickerV3_5101", "entityname8D74E2470C544C0E9561CCE5B3C2026C", "clipname3", 0)
                FaceMakeupFunc.hideMakeup(this, "FaceMakeupV2_2999", "mask2999")
                FaceMakeupFunc.hideMakeup(this, "FaceMakeupV2_2999", "mouth_part2998")
                FaceMakeupFunc.hideMakeup(this, "FaceMakeupV2_2999", "eye_part2997")
                FaceMakeupFunc.hideMakeup(this, "FaceMakeupV2_2999", "mask2996")
                FaceMakeupFunc.playMakeup(this, "FaceMakeupV2_2999", "mask2999")
                FaceMakeupFunc.playMakeup(this, "FaceMakeupV2_2999", "mouth_part2998")
                FaceMakeupFunc.playMakeup(this, "FaceMakeupV2_2999", "eye_part2997")
                FaceMakeupFunc.playMakeup(this, "FaceMakeupV2_2999", "mask2996")
                CommonFunc.setFeatureEnabled(this, "Filter_2998", true)
            end
        end
        return true
    end,
    }

