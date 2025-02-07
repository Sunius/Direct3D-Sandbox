﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ManagedPostProcessor
{
    public static class FontCreator
    {
        const int kMaxFontWidth = 4096;
        const int BitmapSize = 1024;
        const int characterRangeFrom = 32;
        const int characterRangeTo = 128;

        static Brush ForegroundBrush = new SolidBrush(Color.White);
        static StringFormat NoFontFallbackFormat;

        static FontCreator()
        {
            NoFontFallbackFormat = (StringFormat)StringFormat.GenericTypographic.Clone();
            NoFontFallbackFormat.FormatFlags |= StringFormatFlags.NoFontFallback;
        }

        public static void CreateFont(string fontName, float fontSize, string outputDirectory)
        {
            fontSize *= 96.0f / 72.0f;  // Converts points to pixels

            using (var font = new Font(fontName, fontSize, FontStyle.Regular, GraphicsUnit.Pixel))
            {
                Console.WriteLine("Loading font {0}. Loaded font name: {1}.", fontName, font.FontFamily.Name);

                using (var bitmap = new Bitmap(BitmapSize, BitmapSize, PixelFormat.Format32bppArgb))
                {
                    using (var graphics = Graphics.FromImage(bitmap))
                    {
                        graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
                        graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;

                        if (fontSize > 60.0f)
                        {
                            graphics.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
                        }
                        else
                        {
                            graphics.TextRenderingHint = TextRenderingHint.ClearTypeGridFit;
                        }
                        
                        var characters = ImportFont(font, bitmap, graphics);
                        var lineSpace = font.GetHeight();

                        int fontBitmapWidth, fontBitmapHeight;
                        var fontBitmap = PackCharacters(characters, (int)lineSpace, out fontBitmapWidth, out fontBitmapHeight);                        
                        SaveFont(characters, lineSpace, fontBitmapWidth, fontBitmapHeight, fontBitmap, Path.Combine(outputDirectory, fontName + ".font"));
                    }
                }
            }
        }

        private static CharacterGlyph[] ImportFont(Font font, Bitmap stagingBitmap, Graphics stagingGraphics)
        {
            var characters = new CharacterGlyph[characterRangeTo - characterRangeFrom];

            for (int i = characterRangeFrom; i < characterRangeTo; i++)
            {
                characters[i - characterRangeFrom] = ImportCharacter((char)i, font, stagingBitmap, stagingGraphics);
            }

            return characters;
        }

        private static CharacterGlyph ImportCharacter(char character, Font font, Bitmap stagingBitmap, Graphics stagingGraphics)
        {
            var str = character.ToString();
            var characterSize = stagingGraphics.MeasureString(str, font, Point.Empty, NoFontFallbackFormat);
            var abcSpacing = CalculateABCSpacing(character, font, stagingGraphics);

            uint width = (uint)Math.Ceiling(abcSpacing.A + abcSpacing.B + abcSpacing.C);
            uint height = (uint)Math.Ceiling(characterSize.Height);
            Debug.Assert(width <= BitmapSize && height < BitmapSize);

            stagingGraphics.Clear(Color.Black);
            stagingGraphics.DrawString(str, font, ForegroundBrush, 0, 0, NoFontFallbackFormat);
            stagingGraphics.Flush();

            var characterBitmap = stagingBitmap.Clone(new Rectangle(0, 0, (int)width, (int)height), PixelFormat.Format32bppArgb);

            return new CharacterGlyph(character, characterBitmap, width, height);
        }

        // Calculate ABC spacing
        // http://msdn.microsoft.com/en-us/library/windows/desktop/dd162454(v=vs.85).aspx
        private static ABCSpacing CalculateABCSpacing(char character, Font font, Graphics stagingGraphics)
        {
            var hdc = stagingGraphics.GetHdc();

            try
            {
                var hFont = font.ToHfont();

                try
                {
                    var oldFont = GDI.SelectObject(hdc, hFont);

                    try
                    {
                        var abcSpacing = new ABCSpacing[1];
                        var result = GDI.GetCharABCWidthsFloat(hdc, character, character, abcSpacing);

                        if (!result)
                        {
                            throw new Exception("GetCharABCWidthsFloat failed!");
                        }

                        return abcSpacing[0];
                    }
                    finally
                    {
                        GDI.SelectObject(hdc, oldFont);
                    }
                }
                finally
                {
                    GDI.DeleteObject(hFont);
                }
            }
            finally
            {
                stagingGraphics.ReleaseHdc(hdc);
            }
        }

        static byte[] PackCharacters(CharacterGlyph[] characters, int lineSpace, out int bitmapWidth, out int bitmapHeight)
        {
            var offsetX = 0u;
            var offsetY = 0u;

            bitmapWidth = 0;
            bitmapHeight = characters.Max(x => x.Bitmap.Height);

            int currentRowWidth = 0;

            for (int i = 0; i < characters.Length; i++)
            {
                currentRowWidth += characters[i].Bitmap.Width;

                if (currentRowWidth > kMaxFontWidth)
                {
                    currentRowWidth = characters[i].Bitmap.Width;
                    bitmapHeight += lineSpace;
                }

                if (currentRowWidth > bitmapWidth)
                {
                    bitmapWidth = currentRowWidth;
                }                
            }

            var bitmap = new byte[4 * bitmapHeight * bitmapWidth];
        
            for (int i = 0; i < characters.Length; i++)
            {
                var sourceBitmap = characters[i].Bitmap;
                var sourceBitmapData = sourceBitmap.LockBits(new Rectangle(0, 0, sourceBitmap.Width, sourceBitmap.Height), 
                        ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);

                if (offsetX + (uint)characters[i].Bitmap.Width > kMaxFontWidth)
                {
                    offsetX = 0;
                    offsetY += (uint)lineSpace;
                }

                var offset = offsetY * bitmapWidth + offsetX;
                for (int u = 0; u < sourceBitmap.Height; u++)
                {
                    unsafe
                    {
                        var sourceLine = (byte*)sourceBitmapData.Scan0.ToPointer() + sourceBitmapData.Stride * u;

                        for (int v = 0; v < 4 * sourceBitmap.Width; v++)
                        {
                            bitmap[4 * u * bitmapWidth + v + 4 * offset] = *sourceLine;
                            sourceLine++;
                        }
                    }
                }

                characters[i].Bitmap.UnlockBits(sourceBitmapData);

                characters[i].XOffset = offsetX;
                characters[i].YOffset = offsetY;
                offsetX += (uint)characters[i].Bitmap.Width;
            }

            GrayToAlpha(bitmap);
            //SaveAsBitmap(bitmapWidth, bitmapHeight, bitmap);
            return bitmap;
        }

        static unsafe void SaveAsBitmap(int width, int height, byte[] data)
        {
            var bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb);
            var bitmapData = bitmap.LockBits(new Rectangle(0, 0, width, height), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);

            Marshal.Copy(data, 0, bitmapData.Scan0, 4 * width * height);

            bitmap.UnlockBits(bitmapData);

            var temp = Path.GetTempFileName();
            File.Delete(temp);            

            bitmap.Save(Path.GetFileName(Path.ChangeExtension(temp, ".png")));
        }

        private static void GrayToAlpha(byte[] bitmapData)
        {
            int size = bitmapData.Length / 4;

            for (int i = 0; i < size; i++)
            {
                bitmapData[4 * i + 3] = (byte)Math.Ceiling(0.2126 * bitmapData[4 * i + 2] + 0.7152 * bitmapData[4 * i + 1] + 0.0722 * bitmapData[4 * i]);
            }
        }

        // Binary format:
        // 4 bytes - bitmap width
        // 4 bytes - bitmap height
        //      * - bitmap data
        // 4 bytes - number of characters
        //      1 byte - character
        //      4 bytes - xOffset
        //      4 bytes - yOffset
        //      4 bytes - characterWidth
        //      4 bytes - characterHeight
        // 4 bytes - font line spacing
        private static void SaveFont(CharacterGlyph[] characters, float lineSpace, int bitmapWidth, int bitmapHeight, byte[] bitmapData, string outputPath)
        {
            using (var fileStream = new FileStream(outputPath, FileMode.Create))
            {
                using (var writer = new BinaryWriter(fileStream))
                {
                    writer.Write(bitmapWidth);
                    writer.Write(bitmapHeight);
                    writer.Write(bitmapData);

                    writer.Write(characters.Length);

                    foreach (var character in characters)
                    {
                        writer.Write((byte)character.Character);
                        writer.Write(character.XOffset);
                        writer.Write(character.YOffset);
                        writer.Write(character.CharacterWidth);
                        writer.Write(character.CharacterHeight);
                    }

                    writer.Write((uint)lineSpace);
                }
            }
        }

        static class GDI
        {
            [DllImport("gdi32.dll")]
            public static extern IntPtr SelectObject(IntPtr hdc, IntPtr hObject);

            [DllImport("gdi32.dll")]
            public static extern bool DeleteObject(IntPtr hObject);

            [DllImport("gdi32.dll", CharSet = CharSet.Unicode)]
            public static extern bool GetCharABCWidthsFloat(IntPtr hdc, uint firstChar, uint lastChar, [Out] ABCSpacing[] abcSpacingPtr);
        }
    }
}