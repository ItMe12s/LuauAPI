from __future__ import annotations

import unittest
from helpers import (
    Arg,  # type: ignore[import-unresolved]
    Class,  # type: ignore[import-unresolved]
    Method,  # type: ignore[import-unresolved]
    _emit_class_file,  # type: ignore[import-unresolved]
    all_platforms,  # type: ignore[import-unresolved]
)


class FmodBindingTests(unittest.TestCase):
    def test_fmod_opaque_handle_binding_emits_lightuserdata(self) -> None:
        ccobject = Class(name="CCObject", namespace="cocos2d")
        cls = Class(
            name="FMODAudioEngine",
            bases=["CCObject"],
            attributes=["link(win)"],
            methods=[
                Method(
                    name="stopChannel",
                    ret="void",
                    args=[Arg("FMOD::Channel*", "channel")],
                    platforms=all_platforms("0x1"),
                ),
                Method(
                    name="getActiveMusicChannel",
                    ret="FMOD::Channel*",
                    args=[],
                    platforms=all_platforms("0x2"),
                ),
                Method(
                    name="printResult",
                    ret="void",
                    args=[Arg("FMOD_RESULT", "result")],
                    platforms=all_platforms("0x3"),
                ),
                Method(
                    name="channelLinkSound",
                    ret="void",
                    args=[Arg("int", "id"), Arg("FMODSound*", "sound")],
                    platforms=all_platforms("0x4"),
                ),
                Method(
                    name="preloadEffect",
                    ret="FMODSound*",
                    args=[Arg("gd::string", "path")],
                    platforms=all_platforms("0x5"),
                ),
            ],
        )
        objects = {"CCObject": ccobject, "FMODAudioEngine": cls}

        text = _emit_class_file(
            cls,
            {m.name: [m] for m in cls.methods},
            [],
            [],
            objects,
            set(),
            1,
            "win",
        )

        self.assertIn("self->stopChannel(arg0)", text)
        self.assertIn("static_cast<FMOD::Channel*>(lua_touserdata", text)
        self.assertIn("self->channelLinkSound(arg0, arg1)", text)
        self.assertIn("static_cast<FMODSound*>(lua_touserdata", text)
        self.assertIn("auto result = self->getActiveMusicChannel();", text)
        self.assertIn("lua_pushlightuserdata(L, result)", text)
        self.assertIn("static_cast<FMOD_RESULT>", text)
        self.assertIn("auto result = self->preloadEffect(arg0);", text)
        self.assertNotIn("Usertype<FMOD::Channel>", text)
        self.assertNotIn("pushBorrowed", text)
